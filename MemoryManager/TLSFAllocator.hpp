#pragma once

#include "BitSet64.h"
#include "PageAllocator.hpp"

#define PS sizeof(char*)
#define SIZE_TRASHHOLD 2032 + sizeof(used_block_header)
#define MAX_VALUE 4'227'858'432

struct TLSFAllocator
{
    struct free_block_header
    {
        // [0]F [1]T
        char bytes[2];
        uint32_t _size;
        free_block_header* prev_phys_block;

        free_block_header* next_free;
        free_block_header* prev_free;

        // 1 - free
        // 0 - used
        inline bool is_free()
        {
            return bytes[0] == 1;
        }
        inline void set_free()
        {
            bytes[0] = 1;
        }
        inline void set_used()
        {
            bytes[0] = 0;
        }

        // 1 - last
        // 0 - not last
        inline bool is_last_physical_block()
        {
            return bytes[1] == 1;
        }
        inline void set_last_physical_block()
        {
            bytes[1] = 1;
        }
        inline void set_not_last_physical_block()
        {
            bytes[1] = 0;
        }
    };

    struct used_block_header
    {
        // [0]F  1:free, 0:used
        // [1]T  1:last, 0:not last
        char bytes[2];
        uint32_t _size;
        free_block_header* prev_phys_block;

        // 1 - free
        // 0 - used
        inline bool is_free()
        {
            return bytes[0] == 1;
        }
        inline void set_free()
        {
            bytes[0] = 1;
        }
        inline void set_used()
        {
            bytes[0] = 0;
        }

        // 1 - last
        // 0 - not last
        inline bool is_last_physical_block()
        {
            return bytes[1] == 1;
        }
        inline void set_last_physical_block()
        {
            bytes[1] = 1;
        }
        inline void set_not_last_physical_block()
        {
            bytes[1] = 0;
        }
    };

    uint32_t m_FLI;
    uint32_t m_SLI;  // Power of two in range[1,32]
    const uint32_t m_MBS;

    Fleur::Core::BitSet64 m_FL_Bitmap;
    Fleur::Core::BitSet64 m_SL_Bitmap[32];

    free_block_header* m_FreeListHead[32][32]{nullptr};

    PageAllocator* m_PageAlloc;
    unsigned char* m_EndOfMemory;

    TLSFAllocator(PageAllocator* alloc, uint32_t sli, uint32_t mbs, unsigned char* memoryEnd)
        : m_FLI(32)
        , m_SLI(sli)
        , m_MBS(mbs)
        , m_FL_Bitmap(32)
        , m_PageAlloc(alloc)
        , m_EndOfMemory(memoryEnd)
    {
        // Second level bitmap initialization
        for (uint32_t i = 0; i < m_FLI; i++)
        {
            m_SL_Bitmap[i] = Fleur::Core::BitSet64(static_cast<uint8_t>(pow(2, m_SLI)));
        }
    }

    template <typename T, uint32_t ALign = 0>
    [[nodiscard]] T* allocate(uint32_t count = 1)
    {
        uint32_t firstIndex = 0, secondIndex = 0;
        used_block_header* foundBlock = nullptr;
        free_block_header* remainingBlock = nullptr;

        size_t size = sizeof(T) * count + sizeof(used_block_header);
        uint32_t originalSize = size;
        foundBlock = get_block(size, &firstIndex, &secondIndex);

        MM_DEBUG_BREAK(foundBlock->_size < originalSize)

        if (foundBlock->_size - size > SIZE_TRASHHOLD)
        {
            MM_DEBUG_BREAK(foundBlock->_size < originalSize)

            remainingBlock = split(foundBlock, size);
            mapping(remainingBlock->_size, &firstIndex, &secondIndex);
            insert(remainingBlock, firstIndex, secondIndex);
        }
        MM_DEBUG_BREAK(foundBlock->_size < originalSize)

        unsigned char* returnPtr = TOCHARPTR(foundBlock) + sizeof(used_block_header);
        return reinterpret_cast<T*>(returnPtr);
    }

    template <typename T>
    void deallocate(void* ptr, uint32_t count = 1)
    {
        MM_DEBUG_BREAK(ptr == nullptr)

        unsigned char* bytePtr = reinterpret_cast<unsigned char*>(ptr);
        free_block_header* block = reinterpret_cast<free_block_header*>(bytePtr - sizeof(used_block_header));

        MM_DEBUG_BREAK(block->_size > MAX_VALUE || block->_size == 0)

        block->set_free();

        free_block_header* mergedBlock = merge_left(block);
        merge_right(mergedBlock);

        uint32_t firstIndex, secondIndex = 0;
        if (Fleur::Core::bit_scan_reverse(mergedBlock->_size, &firstIndex))
        {
            sli_no_rounding(mergedBlock->_size, &firstIndex, &secondIndex);
            free_block(mergedBlock, firstIndex, secondIndex);
        }
    }

private:
    void mapping(uint32_t size, uint32_t* fl, uint32_t* sl)
    {
        if (Fleur::Core::bit_scan_reverse(size, fl))
        {
            sli_no_rounding(size, fl, sl);
        }
    }

    inline void SLI(uint32_t* size, uint32_t* fl, uint32_t* sl)
    {
        *size = *size + (1u << (*fl - m_SLI)) - 1;
        Fleur::Core::bit_scan_reverse(*size, fl);
        *sl = (*size >> (*fl - m_SLI)) - (1u << m_SLI);
    }
    inline void sli_no_rounding(uint32_t size, uint32_t* fl, uint32_t* sl)
    {
        Fleur::Core::bit_scan_reverse(size, fl);
        *sl = (size >> (*fl - m_SLI)) - (1u << m_SLI);
    }

    inline bool sl_bitmap_lookup_from(uint32_t fl, uint32_t* sl)
    {
        auto* slBitmap = &m_SL_Bitmap[fl];
        return slBitmap->scan_forward_from(sl);
    }

    inline free_block_header* request_block(uint32_t size)
    {
        auto ptr = m_PageAlloc->allocate_pages_size(size, nullptr);
        MM_DEBUG_BREAK(ptr == nullptr);

        auto block = reinterpret_cast<free_block_header*>(ptr);
        block->_size = size;
        block->set_free();
        block->set_last_physical_block();
        block->prev_phys_block = nullptr;
        block->prev_free = nullptr;
        block->next_free = nullptr;

        return block;
    }

    inline used_block_header* use_block(uint32_t fli, uint32_t sli)
    {
        MM_DEBUG_BREAK(m_FreeListHead[fli][sli] == nullptr)

        free_block_header* currentBlock = m_FreeListHead[fli][sli];
        free_block_header* nextBlock = currentBlock->next_free;

        MM_DEBUG_BREAK(currentBlock->_size < SIZE_TRASHHOLD || currentBlock->_size > MAX_VALUE)

        m_FreeListHead[fli][sli] = currentBlock->next_free;
        if (nextBlock)
            nextBlock->prev_free = nullptr;
        else
        {
            m_SL_Bitmap[fli].ClearBit(static_cast<uint8_t>(sli));
            if (!m_SL_Bitmap[fli].ScanFirstSetForward(nullptr))
                m_FL_Bitmap.ClearBit(static_cast<uint8_t>(fli));
        }

        currentBlock->set_used();
        currentBlock->next_free = nullptr;

        MM_DEBUG_BREAK(currentBlock->_size < SIZE_TRASHHOLD || currentBlock->_size > MAX_VALUE)

        return reinterpret_cast<used_block_header*>(currentBlock);
    }

    inline used_block_header* request_and_use_block(uint32_t size, uint32_t fli, uint32_t sli)
    {
        free_block_header* block = request_block(size);
        insert(block, fli, sli);
        return use_block(fli, sli);
    }

    inline void free_block(free_block_header* deallocatedBlock, uint32_t fli, uint32_t sli)
    {
        m_FL_Bitmap.SetBit(static_cast<uint8_t>(fli));
        m_SL_Bitmap[fli].SetBit(static_cast<uint8_t>(sli));

        free_block_header* currentHead = m_FreeListHead[fli][sli];
        if (!currentHead)
        {
            m_FreeListHead[fli][sli] = deallocatedBlock;
            deallocatedBlock->next_free = nullptr;
        }
        else
        {
            currentHead->prev_free = deallocatedBlock;
            deallocatedBlock->next_free = currentHead;

            m_FreeListHead[fli][sli] = deallocatedBlock;
        }
        deallocatedBlock->prev_free = nullptr;
    }

    inline free_block_header* search_suitable_block(uint32_t* fl, uint32_t* sl)
    {
        uint32_t tempBitmap = m_SL_Bitmap[*fl].Get() & (0xFFFFFFFF << *sl);
        if (tempBitmap != 0)
            Fleur::Core::bit_scan_reverse(tempBitmap, sl);
        else
        {
            tempBitmap = m_FL_Bitmap.Get() & (0xFFFFFFFF << (*fl + 1));
            if (tempBitmap == 0)
                return nullptr;

            Fleur::Core::bit_scan_reverse(tempBitmap, fl);
            uint32_t tempSliBitmap = static_cast<uint32_t>(m_SL_Bitmap[*fl].Get());
            Fleur::Core::bit_scan_reverse(tempSliBitmap, sl);
        }
        auto block = m_FreeListHead[*fl][*sl];

        MM_DEBUG_BREAK(block->_size < SIZE_TRASHHOLD || block->_size > 4227858432)

        return block;
    }

    inline used_block_header* get_block(uint32_t size, uint32_t* fl, uint32_t* sl)
    {
        free_block_header* foundBlock = nullptr;

        if (Fleur::Core::bit_scan_reverse(size, fl))
        {
            SLI(&size, fl, sl);
            foundBlock = search_suitable_block(fl, sl);
            if (foundBlock)
            {
                return use_block(*fl, *sl);
            }
        }

        if (!foundBlock)
            return request_and_use_block(size, *fl, *sl);
    }

    inline free_block_header* split(used_block_header* usedBlock, uint32_t usedSize)
    {
        uint32_t remainingSize = usedBlock->_size - usedSize;

        usedBlock->_size = usedSize;
        usedBlock->set_not_last_physical_block();

        unsigned char* remainingBlockBytePtr = reinterpret_cast<unsigned char*>(usedBlock) + usedSize;

        MM_DEBUG_BREAK(remainingBlockBytePtr - reinterpret_cast<unsigned char*>(usedBlock) < SIZE_TRASHHOLD)

        free_block_header* remainingBlock = reinterpret_cast<free_block_header*>(remainingBlockBytePtr);

        remainingBlock->_size = remainingSize;
        remainingBlock->set_free();
        remainingBlock->set_last_physical_block();
        remainingBlock->prev_phys_block = reinterpret_cast<free_block_header*>(usedBlock);
        remainingBlock->next_free = nullptr;
        remainingBlock->prev_free = nullptr;

        MM_DEBUG_BREAK(remainingBlock->_size < SIZE_TRASHHOLD || remainingBlock->_size > MAX_VALUE)

        return remainingBlock;
    }

    inline void insert(free_block_header* block, uint32_t fli, uint32_t sli)
    {
        MM_DEBUG_BREAK(block->_size < SIZE_TRASHHOLD || block->_size > MAX_VALUE)

        m_FL_Bitmap.SetBit(static_cast<uint8_t>(fli));
        m_SL_Bitmap[fli].SetBit(static_cast<uint8_t>(sli));

        free_block_header* currentBlock = m_FreeListHead[fli][sli];
        block->next_free = currentBlock;
        block->prev_free = nullptr;
        if (currentBlock)
            currentBlock->prev_free = block;

        m_FreeListHead[fli][sli] = block;
    }

    inline free_block_header* merge_left(free_block_header* currentBlock)
    {
        if (currentBlock->prev_phys_block)
            MM_DEBUG_BREAK((uintptr_t)currentBlock->prev_phys_block < SIZE_TRASHHOLD)

        if (currentBlock->prev_phys_block && currentBlock->prev_phys_block->is_free())
        {
            free_block_header* prevBlock = currentBlock->prev_phys_block;

            uint32_t fli = 0, sli = 0;
            mapping(prevBlock->_size, &fli, &sli);

            use_block(fli, sli);
            prevBlock->set_free();
            prevBlock->prev_free = nullptr;

            MM_DEBUG_BREAK(prevBlock->_size < SIZE_TRASHHOLD || prevBlock->_size > MAX_VALUE)
            MM_DEBUG_BREAK(currentBlock->_size < SIZE_TRASHHOLD || currentBlock->_size > MAX_VALUE)

            prevBlock->_size += currentBlock->_size;

            MM_DEBUG_BREAK(prevBlock->_size < SIZE_TRASHHOLD || prevBlock->_size > MAX_VALUE)

            if (!currentBlock->is_last_physical_block())
            {
                unsigned char* rightBlockBytePtr = reinterpret_cast<unsigned char*>(currentBlock) + currentBlock->_size;
                if (rightBlockBytePtr < m_EndOfMemory)
                {
                    used_block_header* rightBlock = reinterpret_cast<used_block_header*>(rightBlockBytePtr);
                    rightBlock->prev_phys_block = prevBlock;
                    prevBlock->set_not_last_physical_block();
                    MM_DEBUG_BREAK(rightBlock->_size < SIZE_TRASHHOLD || rightBlock->_size > 4227858432)
                }
                else
                {
                    MM_DEBUG_BREAK(rightBlockBytePtr >= m_EndOfMemory)
                }
            }
            else
            {
                prevBlock->set_last_physical_block();
            }

            currentBlock->prev_phys_block = nullptr;
            currentBlock->_size = 0;

            return prevBlock;
        }
        else
        {
            currentBlock->set_free();
            return currentBlock;
        }
    }

    inline void merge_right(free_block_header* block)
    {
        if (!block->is_last_physical_block())
        {
            unsigned char* blockBytePtr = reinterpret_cast<unsigned char*>(block);
            free_block_header* rightBlock = reinterpret_cast<free_block_header*>(blockBytePtr + block->_size);

            MM_DEBUG_BREAK(rightBlock->_size < SIZE_TRASHHOLD || rightBlock->_size > MAX_VALUE)

            if (rightBlock->is_free())
            {
                uint32_t fli = 0, sli = 0;
                mapping(rightBlock->_size, &fli, &sli);

                use_block(fli, sli);

                MM_DEBUG_BREAK(block->_size < SIZE_TRASHHOLD || block->_size > MAX_VALUE)

                block->_size += rightBlock->_size;

                MM_DEBUG_BREAK(block->_size < SIZE_TRASHHOLD || block->_size > MAX_VALUE)

                if (!rightBlock->is_last_physical_block())
                {
                    unsigned char* rightBlockBytePtr = reinterpret_cast<unsigned char*>(rightBlock) + rightBlock->_size;
                    if (rightBlockBytePtr < m_EndOfMemory)
                    {
                        free_block_header* rightAfterRightBlock = reinterpret_cast<free_block_header*>(rightBlockBytePtr);
                        rightAfterRightBlock->prev_phys_block = block;
                    }
                    else
                    {
                        MM_DEBUG_BREAK(rightBlockBytePtr >= m_EndOfMemory)
                    }
                }
                else
                {
                    block->set_last_physical_block();
                }
            }
        }

        block->set_free();
        block->next_free = nullptr;
        block->prev_free = nullptr;
    }

public:
    void GetSnapshot(char*& buffer) const
    {
        buffer += sprintf_s(buffer, 1024l * 1024l * 50, "//-------------------------- TLSF ALLOCATOR ----------------------------\\\n");

        buffer += sprintf_s(buffer, 1024l * 1024l * 50, "FLI:     {%-32s}\n", m_FL_Bitmap.StringRepresentation().c_str());

        uint32_t fl_idx = 0;


        while (m_FL_Bitmap.scan_forward_from(&fl_idx))
        {
            int sl_freeIdx[32]{-1};
            memset(&sl_freeIdx, -1, 32 * sizeof(int));
            uint32_t sl_idx = 0;

            while (m_SL_Bitmap[fl_idx].scan_forward_from(&sl_idx))
            {
                free_block_header* header = m_FreeListHead[fl_idx][sl_idx];

                free_block_header* nextFreeBlock = header->next_free;
                uint32_t freeBlocks = 1;

                while (nextFreeBlock)
                {
                    freeBlocks++;
                    nextFreeBlock = nextFreeBlock->next_free;
                }
                sl_freeIdx[sl_idx] = freeBlocks;
                sl_idx++;
            }
            std::string str;
            for (int i = 31; i >= 0; i--)
            {
                if (sl_freeIdx[i] != -1)
                {
                    str += std::to_string(sl_freeIdx[i]);
                }
                else
                {
                    str += std::to_string(0);
                }
                str += '|';
                if (i == 15)
                    str += '\t';
            }
            buffer += sprintf_s(buffer, 1024l * 1024l * 50, "SLI[%-2d]: {%s}\n", fl_idx, str.c_str());
            fl_idx++;
        }


        buffer += sprintf_s(buffer, 1024l * 1024l * 50, "\n//---------------------- END OF TLSF ALLOCATOR ----------------------------\\ \n\n");
    }
};