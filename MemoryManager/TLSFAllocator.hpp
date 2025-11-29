#pragma once

#include "BitSet64.h"
#include "PageAllocator.hpp"

#define PS sizeof(char*)

struct TLSFAllocator
{
    struct size_and_flags
    {
        char field[8];

        // 1 - free
        // 0 - used
        inline bool is_free()
        {
            return field[0] == 1;
        }
        inline void set_free()
        {
            field[0] = 1;
        }
        inline void set_used()
        {
            field[0] = 0;
        }

        // 1 - last
        // 0 - not last
        inline bool is_last_physical_block()
        {
            return field[1] == 1;
        }
        inline void set_last_pysiacal_block()
        {
            field[1] = 1;
        }
        inline void set_non_last_pysiacal_block()
        {
            field[1] = 0;
        }

        inline void set_size(size_t size)
        {
            *reinterpret_cast<uint32_t*>(field[2]) = size;
        }
        inline uint32_t get_size()
        {
            return *reinterpret_cast<uint32_t*>(field[2]);
        }
    };

    struct free_block_header
    {
        uint64_t size;

        free_block_header* prev_phys_block;

        free_block_header* next_free;
        free_block_header* prev_free;
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

private:
    uint32_t CalculateNextSize(uint32_t i, uint32_t j) const
    {
        MM_DEBUG_BREAK(j > m_SLI);

        // i - index of first array,    range: log2(MBS) <= i < FLI
        // j - index of second array,   range:         0 <= j < 2^SLI
        uint32_t maxSubdivision = pow(2, m_SLI);
        uint32_t currentSubdivision = pow(2, i);
        if (j == maxSubdivision)
        {
            // Last size if a range of sized of current list
        }
    }
};