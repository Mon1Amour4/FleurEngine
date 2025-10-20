#pragma once

#include <intrin.h>

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

// True     = 1 = Occupied
// False    = 0 = Free
inline bool bit_check(uint64_t number, uint8_t n)
{
    return (number & (static_cast<uint64_t>(1) << n)) != 0;
}

inline bool bit_scan_forward(uint32_t number, uint32_t* idx)
{
#if _MSC_VER
    return _BitScanForward(reinterpret_cast<unsigned long*>(idx), number);
#else
    assert(false);
#endif
}
inline bool bit_scan_forward64(uint64_t number, uint32_t* idx)
{
#if _MSC_VER
    return _BitScanForward64(reinterpret_cast<unsigned long*>(idx), number);
#else
    assert(false);
#endif
}

inline bool bit_scan_reverse(uint32_t number, uint32_t* idx)
{
#if _MSC_VER
    return _BitScanReverse(reinterpret_cast<unsigned long*>(idx), number);
#else
    assert(false);
#endif
}
inline bool bit_scan_reverse64(uint64_t number, uint32_t* idx)
{
#if _MSC_VER
    return _BitScanReverse64(reinterpret_cast<unsigned long*>(idx), number);
#else
    assert(false);
#endif
}

#pragma endregion

#pragma region BitSet64
//======================================================================
/*
 * @brief General purpose bits container, stores up to 64 flags. 0 - free, 1 - occupied
 * @details Bits operations sucs as set\check\clear\toggle.
 */
class BitSet64
{
public:
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

private:
    uint64_t m_Bitmap;
    uint8_t m_Bits;
};

std::ostream& operator<<(std::ostream& os, const BitSet64& obj);

#pragma endregion
}  // namespace Fleur::Core