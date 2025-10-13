#include "BitSet64.h"

//======================================================================
// Bit operations
uint64_t Fleur::Core::bit_set(uint64_t number, uint8_t n)
{
    return number | (static_cast<uint64_t>(1) << n);
}
uint64_t Fleur::Core::bit_clear(uint64_t number, uint8_t n)
{
    return number & ~(static_cast<uint64_t>(1) << n);
}
uint64_t Fleur::Core::bit_toggle(uint64_t number, uint8_t n)
{
    return number ^ (static_cast<uint64_t>(1) << n);
}

// True     = 1 = Occupied
// False    = 0 = Free
bool Fleur::Core::bit_check(uint64_t number, uint8_t n)
{
    return (number & (static_cast<uint64_t>(1) << n)) != 0;
}

//======================================================================
Fleur::Core::BitSet64::BitSet64(uint8_t bits)
    : m_Bitmap(0)
    , m_Bits(bits)
{
    assert(m_Bits <= 64);
    assert(m_Bits > 0);

    m_Bitmap = 0;
}

bool Fleur::Core::BitSet64::IsBitOccupied(uint8_t idx) const
{
    return bit_check(m_Bitmap, idx);
}

void Fleur::Core::BitSet64::SetBit(uint8_t idx)
{
    if (idx > m_Bits - 1)
        __debugbreak();

    m_Bitmap = bit_set(m_Bitmap, idx);
}

void Fleur::Core::BitSet64::ClearBit(uint8_t idx)
{
    m_Bitmap = bit_clear(m_Bitmap, idx);
}

void Fleur::Core::BitSet64::ToggleBit(uint8_t idx)
{
    m_Bitmap = bit_toggle(m_Bitmap, idx);
}

bool Fleur::Core::BitSet64::IsFull() const
{
    uint64_t mask = ~0ull >> (64 - m_Bits);
    return m_Bitmap == mask;
}

uint8_t Fleur::Core::BitSet64::UsedBits() const
{
    return 0;
}