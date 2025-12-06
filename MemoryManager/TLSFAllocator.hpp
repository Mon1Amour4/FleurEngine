#pragma once

#include "BitSet64.h"
#include "PageAllocator.hpp"

#define PS sizeof(char*)
#define SIZE_TRASHHOLD 2032

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
        inline void set_last_pysiacal_block(free_block_header* prev)
        {
            bytes[1] = 1;
            prev_phys_block = prev;
        }
        inline void set_not_last_pysiacal_block()
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
        inline void set_last_pysiacal_block()
        {
            bytes[1] = 1;
        }
        inline void set_not_last_pysiacal_block()
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

    TLSFAllocator(PageAllocator* alloc, uint32_t sli, uint32_t mbs)
        : m_FLI(32)
        , m_SLI(sli)
        , m_MBS(mbs)
        , m_FL_Bitmap(32)
        , m_PageAlloc(alloc)
    {
        // Second level bitmap initialization
        for (uint32_t i = 0; i < m_FLI; i++)
        {
            m_SL_Bitmap[i] = Fleur::Core::BitSet64(pow(2, m_SLI));
        }
        size_t listsNumber = pow(2, m_SLI) * (m_FLI - log2(m_MBS));
    }

    template <typename T, uint32_t ALign = 0>
    [[nodiscard]] T* allocate(uint32_t count = 1)
    {
        uint32_t firstIndex = 0, secondIndex = 0;
        used_block_header* foundBlock = nullptr;
        free_block_header* remainingBlock = nullptr;

        size_t size = sizeof(T) * count + sizeof(used_block_header);

        foundBlock = get_block(size, &firstIndex, &secondIndex);
        if (foundBlock->_size - size > SIZE_TRASHHOLD)
        {
            remainingBlock = split(foundBlock, size);
            mapping(remainingBlock->_size, &firstIndex, &secondIndex);
            insert(remainingBlock, firstIndex, secondIndex);
        }

        unsigned char* returnPtr = TOCHARPTR(foundBlock) + sizeof(free_block_header);
        return reinterpret_cast<T*>(returnPtr);
    }

    template <typename T>
    void deallocate(void* ptr, uint32_t count = 1)
    {
        MM_DEBUG_BREAK(ptr == nullptr)

        unsigned char* bytePtr = reinterpret_cast<unsigned char*>(ptr);
        free_block_header* header = reinterpret_cast<free_block_header*>(bytePtr - sizeof(free_block_header));
        header->set_free();

        uint32_t firstIndex, secondIndex = 0;
        if (Fleur::Core::bit_scan_reverse(header->_size, &firstIndex))
        {
            SLI(header->_size, &firstIndex, &secondIndex);
            free_block(header, firstIndex, secondIndex);
        }
    }

private:
    uint32_t CalculateNextSize(uint32_t i, uint32_t j) const
    {
        MM_DEBUG_BREAK(j > m_SLI);

        // i - index of first array,    range: log2(MBS) <= i < FLI
        // j - index of second array,   range:         0 <= j < 2^SLI
        uint32_t maxSubdivision = pow(2, m_SLI);
        uint32_t currentSubdivision = pow(2, i);

        uint32_t currentSize = 0;
        if (j != maxSubdivision)
        {
            currentSize = currentSubdivision + pow(2, i - m_SLI) * j;
        }
        else
        {
            // Last size if a range of sized of current list
            currentSize = pow(2, i + 1) - 1;
        }
        return currentSize;
    }
    uint32_t CalculateStaticDataHeader() const
    {
        return sizeof(free_block_header) + (PS * pow(2, m_SLI) * (m_FLI - log2(m_MBS)));
    }

    void mapping(size_t size, uint32_t* fl, uint32_t* sl)
    {
        if (Fleur::Core::bit_scan_reverse(size, fl))
        {
            SLI(size, fl, sl);
        }
    }

    inline void SLI(size_t size, uint32_t* fl, uint32_t* sl)
    {
        size = size + (1u << (*fl - m_SLI)) - 1;
        Fleur::Core::bit_scan_reverse(size, fl);
        *sl = (size >> (*fl - m_SLI)) - (1u << m_SLI);
    }

    inline bool sl_bitmap_lookup_from(uint32_t fl, uint32_t* sl)
    {
        auto* slBitmap = &m_SL_Bitmap[fl];
        return slBitmap->scan_forward_from(sl);
    }

    inline free_block_header* request_block(size_t size, uint32_t fli, uint32_t sli)
    {
        auto ptr = m_PageAlloc->allocate_pages_size(size, nullptr);
        MM_DEBUG_BREAK(ptr == nullptr);

        auto header = reinterpret_cast<free_block_header*>(ptr);
        header->_size = size;
        header->set_free();
        header->set_last_pysiacal_block(nullptr);

        return header;
    }

    inline used_block_header* use_block(uint32_t fli, uint32_t sli)
    {
        MM_DEBUG_BREAK(m_FreeListHead[fli][sli] == nullptr)

        free_block_header* header = m_FreeListHead[fli][sli];
        m_FreeListHead[fli][sli] = header->next_free;
        if (header->next_free)
        {
            header->next_free->prev_free = nullptr;
            header->next_free = nullptr;
            header->prev_free = nullptr;
        }


        if (!m_FreeListHead[fli][sli])
            m_SL_Bitmap[fli].ClearBit(sli);

        if (!m_SL_Bitmap[fli].ScanFirstSetForward(nullptr))
            m_FL_Bitmap.ClearBit(fli);

        header->set_used();

        return reinterpret_cast<used_block_header*>(header);
    }

    inline used_block_header* request_and_use_block(size_t size, uint32_t fli, uint32_t sli)
    {
        free_block_header* block = request_block(size, fli, sli);
        insert(block, fli, sli);
        return use_block(fli, sli);
    }

    inline void free_block(free_block_header* deallocatedBlock, uint32_t fli, uint32_t sli)
    {
        m_FL_Bitmap.SetBit(fli);
        m_SL_Bitmap[fli].SetBit(sli);

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
        {
            Fleur::Core::bit_scan_reverse(tempBitmap, sl);
        }
        else
        {
            tempBitmap = m_FL_Bitmap.Get() & (0xFFFFFFFF << (*fl + 1));
            Fleur::Core::bit_scan_reverse(tempBitmap, fl);
            uint32_t tempSliBitmap = m_SL_Bitmap[*fl].Get();
            Fleur::Core::bit_scan_reverse(tempSliBitmap, sl);
        }
        return m_FreeListHead[*fl][*sl];
    }

    inline used_block_header* get_block(uint32_t size, uint32_t* fl, uint32_t* sl)
    {
        free_block_header* foundBlock = nullptr;

        if (Fleur::Core::bit_scan_reverse(size, fl))
        {
            SLI(size, fl, sl);
            foundBlock = search_suitable_block(fl, sl);
            if (foundBlock)
                return use_block(*fl, *sl);
        }

        if (!foundBlock)
            return request_and_use_block(size, *fl, *sl);
    }

    inline free_block_header* split(used_block_header* usedBlock, uint32_t usedSize)
    {
        uint32_t remainingSize = usedBlock->_size - usedSize;
        usedBlock->set_not_last_pysiacal_block();

        unsigned char* remainingBlockBytePtr = reinterpret_cast<unsigned char*>(usedBlock) + sizeof(used_block_header) + usedBlock->_size;
        free_block_header* remainingBlock = reinterpret_cast<free_block_header*>(remainingBlockBytePtr);

        remainingBlock->_size = remainingSize;
        remainingBlock->set_free();
        remainingBlock->set_last_pysiacal_block(reinterpret_cast<free_block_header*>(usedBlock));
        remainingBlock->next_free = nullptr;
        remainingBlock->prev_free = nullptr;

        return remainingBlock;
    }

    inline void insert(free_block_header* block, uint32_t fli, uint32_t sli)
    {
        m_FL_Bitmap.SetBit(fli);
        m_SL_Bitmap[fli].SetBit(sli);

        free_block_header* currentBlock = m_FreeListHead[fli][sli];
        block->next_free = currentBlock;
        block->prev_free = nullptr;
        if (currentBlock)
            currentBlock->prev_free = block;

        m_FreeListHead[fli][sli] = block;
    }

public:
    void GetSnapshot(char*& buffer) const
    {
        buffer += std::sprintf(buffer, "//-------------------------- TLSF ALLOCATOR ----------------------------\\\n");

        buffer += std::sprintf(buffer, "FLI:     {%-32s}\n", m_FL_Bitmap.StringRepresentation().c_str());

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
            buffer += std::sprintf(buffer, "SLI[%-2d]: {%s}\n", fl_idx, str.c_str());
            fl_idx++;
        }


        buffer += std::sprintf(buffer, "\n//---------------------- END OF TLSF ALLOCATOR ----------------------------\\ \n\n");
    }
};