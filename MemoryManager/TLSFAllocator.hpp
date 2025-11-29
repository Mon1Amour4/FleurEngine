#pragma once

#include "BitSet64.h"
#include "PageAllocator.hpp"

struct TLSFAllocator
{
    // unsigned char* m_Head;
    uint32_t m_FLI;
    uint32_t m_SLI;  // Power of two in range[1,32]
    const uint32_t m_MBS;

    Fleur::Core::BitSet64 m_FirstLevelBitmap;

    TLSFAllocator(uint32_t sli, uint32_t mbs)
        : m_FLI(32)
        , m_SLI(sli)
        , m_MBS(mbs)
        , m_FirstLevelBitmap(32)
    {
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