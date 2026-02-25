#pragma once

#if _MSC_VER
#include <intrin.h>
#endif

#include <bit>
#include <cassert>
#include <cstdint>
#include <ostream>

namespace Fleur::Core
{
#pragma region Bit operations
//======================================================================
// Bit operations
inline uint64_t bit_set(uint64_t number, uint8_t n)
{
    return number | (static_cast<uint64_t>(1) << n);
}

inline uint64_t bit_clear(uint64_t number, uint8_t n)
{
    return number & ~(static_cast<uint64_t>(1) << n);
}

inline uint64_t bit_toggle(uint64_t number, uint8_t n)
{
    return number ^ (static_cast<uint64_t>(1) << n);
}

// True     = 1 = Set
// False    = 0 = zero
inline bool bit_check(uint64_t number, uint8_t n)
{
    return (number & (static_cast<uint64_t>(1) << n)) != 0;
}

inline bool bit_scan_forward(uint32_t number, uint32_t* idx)
{
    uint32_t fallback = 0;
    if (!idx)
        idx = &fallback;

#if _MSC_VER
    return _BitScanForward(reinterpret_cast<unsigned long*>(idx), number);
#elif defined(__GNUC__) || defined(__clang__)
    if (number == 0)
    {
        *idx = 0;
        return false;
    }
    *idx = uint32_t(__builtin_ctz(number));
    return number != 0;
#else
    assert(false);
#endif
}
inline bool bit_scan_forward64(uint64_t number, uint32_t* idx)
{
    uint32_t fallback = 0;
    if (!idx)
        idx = &fallback;

#if _MSC_VER
    return _BitScanForward64(reinterpret_cast<unsigned long*>(idx), number);
#elif defined(__GNUC__) || defined(__clang__)
    if (number == 0)
    {
        *idx = 0;
        return false;
    }
    *idx = uint64_t(__builtin_ctzll(number));
    return number != 0;
#else
    assert(false);
#endif
}

/**
 * @brief Find the most significant set bit (MSB) in a 32-bit integer.
 *
 * Scans the bits of the input value starting from the most significant bit (bit 31)
 * down to the least significant bit (bit 0). If a set bit (1) is found, the index
 * of that bit is written to @p idx.
 *
 * @param number [in]  The 32-bit value to scan.
 * @param idx    [out] Pointer to an unsigned integer that receives the bit index
 *                     of the highest set bit. The value is undefined if no bits
 *                     are set in @p number.
 *
 * @return true  If at least one bit is set in @p number.
 * @return false If @p number contains no set bits.
 */
inline bool bit_scan_reverse(uint32_t number, uint32_t* idx)
{
#if _MSC_VER
    return _BitScanReverse(reinterpret_cast<unsigned long*>(idx), number);
#elif defined(__GNUC__) || defined(__clang__)
    if (number == 0)
    {
        *idx = 0;
        return false;
    }
    *idx = 31 - uint32_t(__builtin_clz(number));
    return number != 0;
#else
    assert(false);
#endif
}
inline bool bit_scan_reverse64(uint64_t number, uint32_t* idx)
{
#if _MSC_VER
    return _BitScanReverse64(reinterpret_cast<unsigned long*>(idx), number);
#elif defined(__GNUC__) || defined(__clang__)
    if (number == 0)
    {
        *idx = 0;
        return false;
    }
    *idx = 63 - uint64_t(__builtin_clzll(number));
    return number != 0;
#else
    assert(false);
#endif
}

#pragma endregion

#pragma region BitSet64
//======================================================================
/*
 * @brief General purpose bits container, stores up to 64 flags. 0 - free, 1 - occupied
 * @details Bits operations such as set\check\clear\toggle.
 */
class BitSet64
{
public:
    BitSet64();
    BitSet64(uint8_t bits);

    bool IsBitOccupied(uint8_t idx) const;

    void SetBit(uint8_t idx);
    void ClearBit(uint8_t idx);

    void ToggleBit(uint8_t idx);

    bool IsFull() const;

    uint8_t UsedBits() const;

    uint64_t Get() const;

    /*
        If a set bit is found, the bit position of the first set bit is written to the address specified in the first parameter and the function returns true.
        If no bit is found, the function returns false and the value written to the address in the first parameter is undefined.
    */
    bool ScanFirstSetForward(uint32_t* val) const;
    /*
        If a free bit is found, the bit position of the first free bit is written to the address specified in the first parameter and the function returns true.
        If no bit is found, the function returns false and the value written to the address in the first parameter is undefined.
    */
    bool ScanFirstFreeForward(uint32_t* val) const;

    uint8_t MaskCount() const;

    /**
     * @brief Scan the bitmap for the next set bit starting from a given position.
     *
     * Searches the internal bitmap for the first set bit (LSB -> MSB), beginning at
     * bit index @p from. All bits below @p from are ignored. The scan is performed
     * on either a 32-bit or 64-bit portion depending on the configured bitmap size.
     *
     * @param from [in]  Bit index to start scanning from (0-based). Bits before this
     *                   index are skipped.
     * @param idx  [out] Receives the index of the first set bit found at or after
     *                   @p from. The output value is undefined if no bit is set.
     *
     * @return true  If a set bit was found at or after @p from.
     * @return false If no bits are set in the scanned region.
     */
    inline bool scan_forward_from(uint32_t* idx) const
    {
        if (*idx > (uint8_t)(m_Bits - 1))
            return false;

        uint64_t shiftedBitmap = m_Bitmap;
        uint32_t originalIdx = *idx;
        bool res = false;
        if (*idx > 0)
            shiftedBitmap = shiftedBitmap >> (*idx);

        if (m_Bits <= 32)
        {
            res = bit_scan_forward(static_cast<uint32_t>(shiftedBitmap), idx);
        }
        else
        {
            res = bit_scan_forward64(shiftedBitmap, idx);
        }
        if (res)
            *idx += originalIdx;
        else
            *idx = originalIdx;
        return res;
    }

    inline std::string StringRepresentation() const
    {
        std::string str;
        str.reserve(64);

        int bitsCount = MaskCount();

        for (int i = bitsCount; i >= 0; i--)
        {
            const bool bitSet = (m_Bitmap & (1ull << i)) != 0;
            str += bitSet ? '1' : '0';
        }
        return str;
    }

private:
    uint64_t m_Bitmap;
    uint8_t m_Bits;
};

std::ostream& operator<<(std::ostream& os, const BitSet64& obj);

#pragma endregion
}  // namespace Fleur::Core
