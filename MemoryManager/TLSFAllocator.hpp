#pragma once

#include "BitSet64.h"
#include "PageAllocator.hpp"

#define PS sizeof(char*)
#define SIZE_TRASHHOLD 2032 + sizeof(used_block_header)

struct TLSFAllocator
{
    struct free_block_header
    {
        // [0]F [1]T
        char bytes[2];
        int64_t _size;
        size_t id;
        uint8_t m_fli = 0;
        uint8_t m_sli = 0;
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
        int64_t _size;
        size_t id;
        uint8_t m_fli = 0;
        uint8_t m_sli = 0;
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

    static size_t id;
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
            m_SL_Bitmap[i] = Fleur::Core::BitSet64(pow(2, m_SLI));
        }
        size_t listsNumber = pow(2, m_SLI) * (m_FLI - log2(m_MBS));
    }

    template <typename T, uint32_t ALign = 0>
    [[nodiscard]] T* allocate(uint32_t count = 1)
    {
        uint32_t fliAfterSplit = 0;
        uint32_t sliAfterSplit = 0;
        uint32_t firstIndex = 0, secondIndex = 0;
        used_block_header* foundBlock = nullptr;
        free_block_header* remainingBlock = nullptr;

        size_t size = sizeof(T) * count + sizeof(used_block_header);
        uint32_t originalSize = size;
        foundBlock = get_block(size, &firstIndex, &secondIndex);
        MM_DEBUG_BREAK(foundBlock->_size < originalSize)
        uint32_t fliBeforeSplitting = firstIndex;
        uint32_t sliBeforeSplitting = secondIndex;
        uint32_t sizeAfterRounding = size;

        MM_DEBUG_BREAK(foundBlock->_size == -1234)
        MM_DEBUG_BREAK(foundBlock->_size < originalSize)

        bool isSplitted = false;
        uint32_t blockFLI = firstIndex, blockSLI = secondIndex;
        size_t debugOriginalSize = foundBlock->_size;
        if (foundBlock->_size - size > SIZE_TRASHHOLD)
        {
            MM_DEBUG_BREAK(debugOriginalSize != foundBlock->_size)

            remainingBlock = split(foundBlock, size);
            MM_DEBUG_BREAK(foundBlock->_size > debugOriginalSize)
            mapping(remainingBlock->_size, &firstIndex, &secondIndex);
            uint32_t checkSize = pow(2, firstIndex) + (secondIndex * pow(2, firstIndex - 5));
            MM_DEBUG_BREAK(remainingBlock->_size < checkSize)
            insert(remainingBlock, firstIndex, secondIndex);

            // Debug
            MM_DEBUG_BREAK(blockFLI < firstIndex)
            isSplitted = true;
            // End of Debug


            mapping(foundBlock->_size, &fliAfterSplit, &sliAfterSplit);
        }
        MM_DEBUG_BREAK(foundBlock->_size < originalSize)
        unsigned char* returnPtr = TOCHARPTR(foundBlock) + sizeof(used_block_header);

        /* std::ofstream myFile;
         myFile.open("MemorySnapshot.txt", std::ios_base::app);

         char* buffer = new char[2048];
         char* tmp = buffer;
         tmp += sprintf(tmp, "TLSF allocation: ptr {0x%p} \n", (void*)returnPtr);
         tmp += sprintf(tmp, "Found Block ID: %d, Original Size: %d, Size after rounding: %d, count: %d, block before split [%d][%d] -> [%d][%d]\n",
                        foundBlock->id, originalSize, sizeAfterRounding, count, fliBeforeSplitting, sliBeforeSplitting, fliAfterSplit, sliAfterSplit);
         if (isSplitted)
         {
             tmp += sprintf(tmp, "Block was splitted:New block size: %d, Remaining block: id: %d, {0x%p} %d [%d][%d]\n", debugOriginalSize, remainingBlock->id,
                            (void*)remainingBlock, remainingBlock->_size, firstIndex, secondIndex);
         }
         tmp += '\0';

         myFile << buffer;
         myFile.close();
         delete[] buffer;*/
        return reinterpret_cast<T*>(returnPtr);
    }

    template <typename T>
    void deallocate(void* ptr, uint32_t count = 1)
    {
        MM_DEBUG_BREAK(ptr == nullptr)

        unsigned char* bytePtr = reinterpret_cast<unsigned char*>(ptr);
        free_block_header* block = reinterpret_cast<free_block_header*>(bytePtr - sizeof(used_block_header));

        MM_DEBUG_BREAK(block->_size > 5'000'000 || block->_size == 0)

        block->set_free();

        // Debug
        uint32_t mergedLeftSize = 0;
        uint32_t mergedRightSize = 0;
        uint32_t originalSize = block->_size;
        uint32_t originalID = block->id;
        bool mergedLeft = false;
        bool mergeRight = false;
        void* originalPtr = (void*)block;
        void* leftPtr = nullptr;
        void* rightPtr = nullptr;
        // End debug

        free_block_header* mergedBlock = merge_left(block);

        // Debug
        if (mergedBlock != block)
        {
            mergedLeft = true;
            mergedLeftSize = mergedBlock->_size;
            leftPtr = (void*)mergedBlock;
        }
        // end debug

        merge_right(mergedBlock);

        // debug
        if ((!mergedLeft && mergedBlock->_size > originalSize) || (mergedLeftSize + originalSize > mergedBlock->_size))
        {
            mergeRight = true;
            mergedRightSize = mergedBlock->_size;
            rightPtr = mergedBlock;
        }

        uint32_t firstIndex, secondIndex = 0;
        if (Fleur::Core::bit_scan_reverse(mergedBlock->_size, &firstIndex))
        {
            sli_no_rounding(mergedBlock->_size, &firstIndex, &secondIndex);
            free_block(mergedBlock, firstIndex, secondIndex);
        }

        /*std::ofstream myFile;
        myFile.open("MemorySnapshot.txt", std::ios_base::app);

        char* buffer = new char[2048];
        char* tmp = buffer;
        tmp += sprintf(tmp, "TLSF dealloction: ptr {0x%p}, count: %d, Original block ID:%d, size: %d, original block {0x%d}\n", (void*)ptr, count, originalID,
                       originalSize, originalPtr);
        if (mergedLeft)
        {
            tmp += sprintf(tmp, "Block was merged left, size: %d, block ptr {0x%p}\n", mergedLeftSize, leftPtr);
        }
        if (mergeRight)
        {
            tmp += sprintf(tmp, "\nBlock was merged right, size: after merge: %d, right block ptr{0x%p}\n", mergedBlock, rightPtr);
        }

        tmp += '\0';
        myFile << buffer;
        myFile.close();
        delete[] buffer;*/
    }

private:
    void mapping(size_t size, uint32_t* fl, uint32_t* sl)
    {
        if (Fleur::Core::bit_scan_reverse(size, fl))
        {
            sli_no_rounding(size, fl, sl);
        }
    }

    inline void SLI(uint32_t* size, uint32_t* fl, uint32_t* sl)
    {
        // uint32_t subsize = 1u << (*fl - m_SLI);

        // uint32_t rounded = (*size + subsize - 1) & ~(subsize - 1);

        //*sl = (rounded - (1u << *fl)) / subsize;
        *size = *size + (1u << (*fl - m_SLI)) - 1;
        Fleur::Core::bit_scan_reverse(*size, fl);
        *sl = (*size >> (*fl - m_SLI)) - (1u << m_SLI);
    }
    inline void sli_no_rounding(size_t size, uint32_t* fl, uint32_t* sl)
    {
        Fleur::Core::bit_scan_reverse(size, fl);
        *sl = (size >> (*fl - m_SLI)) - (1u << m_SLI);
    }

    inline bool sl_bitmap_lookup_from(uint32_t fl, uint32_t* sl)
    {
        auto* slBitmap = &m_SL_Bitmap[fl];
        return slBitmap->scan_forward_from(sl);
    }

    inline free_block_header* request_block(size_t size)
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

        MM_DEBUG_BREAK(currentBlock->_size < SIZE_TRASHHOLD || currentBlock->_size > 4227858432)

        m_FreeListHead[fli][sli] = currentBlock->next_free;
        if (nextBlock)
            nextBlock->prev_free = nullptr;
        else
        {
            m_SL_Bitmap[fli].ClearBit(sli);
            if (!m_SL_Bitmap[fli].ScanFirstSetForward(nullptr))
                m_FL_Bitmap.ClearBit(fli);
        }

        currentBlock->set_used();
        currentBlock->next_free = nullptr;

        MM_DEBUG_BREAK(currentBlock->_size < SIZE_TRASHHOLD || currentBlock->_size > 4227858432)

        return reinterpret_cast<used_block_header*>(currentBlock);
    }

    inline used_block_header* request_and_use_block(size_t size, uint32_t fli, uint32_t sli)
    {
        free_block_header* block = request_block(size);
        block->id = id;
        id++;
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
            Fleur::Core::bit_scan_reverse(tempBitmap, sl);
        else
        {
            tempBitmap = m_FL_Bitmap.Get() & (0xFFFFFFFF << (*fl + 1));
            if (tempBitmap == 0)
                return nullptr;

            Fleur::Core::bit_scan_reverse(tempBitmap, fl);
            uint32_t tempSliBitmap = m_SL_Bitmap[*fl].Get();
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
                MM_DEBUG_BREAK(foundBlock->_size == -1234)
                return use_block(*fl, *sl);
            }
        }

        if (!foundBlock)
            return request_and_use_block(size, *fl, *sl);
    }

    inline free_block_header* split(used_block_header* usedBlock, uint32_t usedSize)
    {
        // debug
        MM_DEBUG_BREAK(usedBlock->_size <= usedSize)
        size_t debugSize = usedBlock->_size - usedSize;
        uint32_t usedBlockFLI = 0, usedBlockSLI = 0;
        mapping(usedBlock->_size, &usedBlockFLI, &usedBlockSLI);
        uint32_t remainingBlockFLI = 0, remainingBlockSLI = 0;
        mapping(debugSize, &remainingBlockFLI, &remainingBlockSLI);
        MM_DEBUG_BREAK(usedBlockFLI < remainingBlockFLI);
        // end of debug

        int remainingSize = usedBlock->_size - usedSize;

        MM_DEBUG_BREAK(debugSize != remainingSize);
        MM_DEBUG_BREAK(remainingSize == -1234)

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
        remainingBlock->id = id;
        id++;

        MM_DEBUG_BREAK(remainingBlock->_size < SIZE_TRASHHOLD)
        MM_DEBUG_BREAK(remainingBlock->_size > 4227858432)
        return remainingBlock;
    }

    inline void insert(free_block_header* block, uint32_t fli, uint32_t sli)
    {
        MM_DEBUG_BREAK(block->_size < SIZE_TRASHHOLD || block->_size > 4227858432)

        m_FL_Bitmap.SetBit(fli);
        m_SL_Bitmap[fli].SetBit(sli);

        free_block_header* currentBlock = m_FreeListHead[fli][sli];
        block->next_free = currentBlock;
        block->prev_free = nullptr;
        if (currentBlock)
            currentBlock->prev_free = block;

        block->m_fli = fli;
        block->m_sli = sli;
        m_FreeListHead[fli][sli] = block;
    }

    inline free_block_header* merge_left(free_block_header* currentBlock)
    {
        if (currentBlock->prev_phys_block)
            MM_DEBUG_BREAK((uintptr_t)currentBlock->prev_phys_block < 2048)

        if (currentBlock->prev_phys_block && currentBlock->prev_phys_block->is_free())
        {
            free_block_header* prevBlock = currentBlock->prev_phys_block;

            uint32_t prevBlockOGSize = prevBlock->_size;
            uint32_t prevBlockOGSID = prevBlock->id;

            uint32_t fli = 0, sli = 0;
            mapping(prevBlock->_size, &fli, &sli);

            MM_DEBUG_BREAK(fli != prevBlock->m_fli || sli != prevBlock->m_sli)

            use_block(fli, sli);
            prevBlock->set_free();
            prevBlock->prev_free = nullptr;

            MM_DEBUG_BREAK(prevBlock->_size < SIZE_TRASHHOLD || prevBlock->_size > 4227858432)
            MM_DEBUG_BREAK(currentBlock->_size < SIZE_TRASHHOLD || currentBlock->_size > 4227858432)

            prevBlock->_size += currentBlock->_size;

            MM_DEBUG_BREAK(prevBlock->_size < SIZE_TRASHHOLD || prevBlock->_size > 4227858432)

            prevBlock->id = id;
            id++;

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
                    MM_DEBUG_BREAK(true)
                }
            }
            else
            {
                prevBlock->set_last_physical_block();
            }

            currentBlock->prev_phys_block = nullptr;
            currentBlock->_size = -1234;

            // Debug
            /* std::ofstream myFile;
             myFile.open("MemorySnapshot.txt", std::ios_base::app);

             char* buffer = new char[2048];
             char* tmp = buffer;
             tmp += sprintf(tmp, "merge_left: currentBlock: {0x%p}, id: %d, size: %d \nWas merged with: {0x%p}, id: %d, size: %d", (void*)currentBlock,
                            currentBlock->id, currentBlock->_size, (void*)prevBlock, prevBlockOGSID, prevBlockOGSize);
             tmp += sprintf(tmp, " into: size: %d, id: %d\n", prevBlock->_size, prevBlock->id);
             tmp += '\0';

             myFile << buffer;
             myFile.close();
             delete[] buffer;*/
            // End of debug

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
        uint32_t blockSize = block->_size;
        uint32_t blockID = block->id;
        uint32_t rightBlockID = 0;
        uint32_t rightBlockSize = 0;

        if (!block->is_last_physical_block())
        {
            unsigned char* blockBytePtr = reinterpret_cast<unsigned char*>(block);
            free_block_header* rightBlock = reinterpret_cast<free_block_header*>(blockBytePtr + block->_size);

            MM_DEBUG_BREAK(rightBlock->_size < SIZE_TRASHHOLD || rightBlock->_size > 4227858432)

            if (rightBlock->is_free())
            {
                rightBlockID = rightBlock->id;
                rightBlockSize = rightBlock->_size;

                uint32_t fli = 0, sli = 0;
                mapping(rightBlock->_size, &fli, &sli);

                MM_DEBUG_BREAK(fli != rightBlock->m_fli || sli != rightBlock->m_sli)

                use_block(fli, sli);

                MM_DEBUG_BREAK(block->_size < SIZE_TRASHHOLD || block->_size > 4227858432)

                block->_size += rightBlock->_size;

                MM_DEBUG_BREAK(block->_size < SIZE_TRASHHOLD || block->_size > 4227858432)

                rightBlock->id = id;
                id++;

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
                        MM_DEBUG_BREAK(true)
                    }
                }
                else
                {
                    block->set_last_physical_block();
                }

                // Debug
                /*std::ofstream myFile;
                myFile.open("MemorySnapshot.txt", std::ios_base::app);

                char* buffer = new char[2048];
                char* tmp = buffer;
                tmp += sprintf(tmp, "merge_right: currentBlock: {0x%p}, id: %d, size: %d \nWas merged with: {0x%p}, id: %d, size: %d", (void*)block, blockID,
                               blockSize, (void*)rightBlock, rightBlockID, rightBlockSize);
                tmp += sprintf(tmp, " into: size: %d, id: %d\n", block->_size, block->id);
                tmp += '\0';

                myFile << buffer;
                myFile.close();
                delete[] buffer;*/
                // End of debug
            }
        }

        block->set_free();
        block->next_free = nullptr;
        block->prev_free = nullptr;
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