#pragma once

#include <intrin.h>

#include <cassert>
#include <cstdint>

namespace Fleur::Core
{
#pragma region Bit operations
//======================================================================
// Bit operations
uint64_t bit_set(uint64_t number, uint8_t n);

uint64_t bit_clear(uint64_t number, uint8_t n);

uint64_t bit_toggle(uint64_t number, uint8_t n);

// True     = 1 = Occupied
// False    = 0 = Free
bool bit_check(uint64_t number, uint8_t n);

bool bit_scan_forward(uint32_t number, uint32_t* idx);
bool bit_scan_forward64(uint64_t number, uint32_t* idx);

bool bit_scan_reverse(uint32_t number, uint32_t* idx);
bool bit_scan_reverse64(uint64_t number, uint32_t* idx);


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

private:
    uint64_t m_Bitmap;
    uint8_t m_Bits;
};
#pragma endregion
}  // namespace Fleur::Core