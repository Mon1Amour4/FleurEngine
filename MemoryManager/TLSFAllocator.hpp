#pragma once

#include "BitSet64.h"
#include "PageAllocator.hpp"

#define PS sizeof(char*)
#define FRAG_LIMIT_PERCENT 0.03f

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
        size_t size = sizeof(T) * count + sizeof(free_block_header);

        uint32_t firstIndex = 0, secondIndex = 0;
        free_block_header* blockHead = nullptr;
        if (Fleur::Core::bit_scan_reverse(size, &firstIndex))
        {
            SLI(size, &firstIndex, &secondIndex);
            blockHead = search_suitable_block(&firstIndex, &secondIndex);
            if (blockHead)
                use_block(firstIndex, secondIndex);
        }

        if (!blockHead)
            blockHead = request_and_use_block(size, firstIndex, secondIndex, nullptr, nullptr);

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
        if (Fleur::Core::bit_scan_reverse(header->get_size(), &firstIndex))
        {
            SLI(header->get_size(), &firstIndex, &secondIndex);
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

    inline free_block_header* search_suitable_block(uint32_t* fl, uint32_t* sl)
    {
        uint32_t tempBitmap = m_SL_Bitmap[*fl].Get() & (0xFFFFFFFF << *sl);
        if (tempBitmap != 0)
        {
            Fleur::Core::bit_scan_reverse(tempBitmap, sl);
        }
        else
        {
            uint8_t sliCap = get_max_sli_within_waste_limit(*fl, *sl);
            if (sliCap == 0)
                return nullptr;

            tempBitmap = m_FL_Bitmap.Get() & (0xFFFFFFFF << (*fl + 1));
            Fleur::Core::bit_scan_reverse(tempBitmap, fl);
            uint32_t tempSliBitmap = m_SL_Bitmap[*fl].Get();
            tempSliBitmap &= (0xffffffff >> (32 - sliCap));
            Fleur::Core::bit_scan_reverse(tempSliBitmap, sl);
        }
        return m_FreeListHead[*fl][*sl];
    }

    uint8_t get_max_sli_within_waste_limit(uint8_t prevFli, uint8_t sli)
    {
        uint32_t nextSubbinSize = 1u << (prevFli + 1 - m_SLI);
        uint32_t blockSize = (1 << prevFli) + (sli * (1 << (prevFli - m_SLI))) + sizeof(free_block_header);
        uint32_t maxSize = blockSize * (1. + FRAG_LIMIT_PERCENT);
        return (maxSize - blockSize) / nextSubbinSize;
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