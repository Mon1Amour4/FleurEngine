#pragma once

#include "BitSet64.h"
#include "PageAllocator.hpp"

#define PS sizeof(char*)

struct TLSFAllocator
{
    struct free_block_header
    {
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
        inline void set_last_pysiacal_block()
        {
            bytes[1] = 1;
        }
        inline void set_non_last_pysiacal_block()
        {
            bytes[1] = 0;
        }

        inline void set_size(size_t size)
        {
            _size = size;
        }
        inline uint32_t get_size()
        {
            return _size;
        }
    };

    struct used_block_header
    {
        uint64_t size;
        free_block_header* prev_phys_block;
    };

    uint32_t m_FLI;
    uint32_t m_SLI;  // Power of two in range[1,32]
    const uint32_t m_MBS;

    Fleur::Core::BitSet64 m_FL_Bitmap;
    Fleur::Core::BitSet64 m_SL_Bitmap[32];

    free_block_header* m_FreeListHead[32][32]{nullptr};

    PageAllocator* m_PageAlloc;

    TLSFAllocator(PageAllocator* alloc, uint32_t sli, uint32_t mbs)
        : m_FLI(21)
        , m_SLI(sli)
        , m_MBS(mbs)
        , m_FL_Bitmap(32)
        , m_PageAlloc(alloc)
    {
        // Second level bitmap initialization
        for (uint32_t i = 0; i < m_FLI; i++)
        {
            m_SL_Bitmap[i] = Fleur::Core::BitSet64(m_SLI);
        }
        size_t listsNumber = pow(2, m_SLI) * (m_FLI - log2(m_MBS));
    }

    template <typename T, uint32_t ALign = 0>
    [[nodiscard]] T* allocate(uint32_t count = 1)
    {
        size_t size = sizeof(T) * count + sizeof(free_block_header);
        uint32_t power = 0;
        Fleur::Core::bit_scan_reverse(size, &power);
        size_t alignedSize = pow(2, power);

        uint32_t firstIndex, secondIndex = 0;
        free_block_header* blockHead = nullptr;
        if (FLI(alignedSize, &firstIndex))
        {
            SLI(alignedSize, firstIndex, &secondIndex);
            free_block_header* blockHead = m_FreeListHead[firstIndex][secondIndex];
            if (!blockHead)
            {
                if (sl_bitmap_lookup_from(firstIndex, &secondIndex))
                {
                    blockHead = m_FreeListHead[firstIndex][secondIndex];
                    if (blockHead)
                        use_block(firstIndex, secondIndex);
                }
            }
        }

        if (!blockHead)
            blockHead = request_and_use_block(alignedSize, firstIndex, secondIndex, nullptr, nullptr);

        unsigned char* returnPtr = TOCHARPTR(blockHead) + sizeof(free_block_header);
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
        if (FLI(header->get_size(), &firstIndex))
        {
            SLI(header->get_size(), firstIndex, &secondIndex);
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
        *sl = (size >> (*fl - m_SLI)) - pow(2, m_SLI);
    }

    inline bool FLI(size_t size, uint32_t* fl)
    {
        return Fleur::Core::bit_scan_reverse(size, fl);
    }
    inline void SLI(size_t size, uint32_t fl, uint32_t* sl)
    {
        *sl = (size >> (fl - m_SLI)) - pow(2, m_SLI);
    }

    /**
     * @brief Scans the SLI bitmap for the given FLI starting from *sl.
     *
     * Uses *sl as the starting SLI index and updates it to the position of the
     * next set bit if found.
     *
     * @param fl  First-level index (FLI).
     * @param sl  Pointer to SLI index (in/out). Must not be nullptr.
     *
     * @return true if a set bit was found, false otherwise.
     */
    inline bool sl_bitmap_lookup_from(uint32_t fl, uint32_t* sl)
    {
        auto* slBitmap = &m_SL_Bitmap[fl];
        return slBitmap->scan_forward_from(sl);
    }

    inline free_block_header* request_block(size_t size, uint32_t fli, uint32_t sli, free_block_header* nextFree, free_block_header* prevFree)
    {
        auto ptr = m_PageAlloc->allocate_pages_size(size, nullptr);
        MM_DEBUG_BREAK(ptr == nullptr);

        auto header = reinterpret_cast<free_block_header*>(ptr);
        header->set_free();
        if (sli != pow(2, m_SLI))
            header->set_non_last_pysiacal_block();
        else
            header->is_last_physical_block();

        header->set_size(size);

        if (sli > 0)
            header->prev_phys_block = m_FreeListHead[fli][sli - 1];
        else
            header->prev_phys_block = nullptr;

        header->next_free = nextFree;
        header->prev_free = prevFree;

        m_FL_Bitmap.SetBit(fli);
        m_SL_Bitmap[fli].SetBit(sli);

        return header;
    }

    inline void use_block(uint32_t fli, uint32_t sli)
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
    }

    inline free_block_header* request_and_use_block(size_t size, uint32_t fli, uint32_t sli, free_block_header* nextFree, free_block_header* prevFree)
    {
        m_FreeListHead[fli][sli] = request_block(size, fli, sli, nextFree, prevFree);
        free_block_header* requestedBlock = m_FreeListHead[fli][sli];
        use_block(fli, sli);
        return requestedBlock;
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
};