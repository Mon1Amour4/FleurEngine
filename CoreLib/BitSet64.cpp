#include "BitSet64.h"

std::ostream& Fleur::Core::operator<<(std::ostream& os, const Fleur::Core::BitSet64& obj)
{
    uint64_t bitsCount = obj.MaskCount();
    uint64_t bitmap = obj.Get();

    char* buffer = new char[bitsCount + 1];
    buffer[bitsCount] = '\0';

    for (size_t i = 0; i < bitsCount; i++)
    {
        const bool bitSet = (bitmap & (1ull << i)) != 0;
        buffer[bitsCount - 1 - i] = bitSet ? '1' : '0';
    }
    os << buffer;

    delete[] buffer;

    return os;
}

//======================================================================
Fleur::Core::BitSet64::BitSet64()
    : m_Bitmap(0)
    , m_Bits(0)
{
}
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
    return static_cast<uint8_t>(std::popcount(m_Bitmap));
}

uint64_t Fleur::Core::BitSet64::Get() const
{
    return m_Bitmap;
}

bool Fleur::Core::BitSet64::ScanFirstSetForward(uint32_t* val) const
{
    if (m_Bits < 32)
        return bit_scan_forward(static_cast<uint32_t>(m_Bitmap), val);

    return bit_scan_forward64(m_Bitmap, val);
}
bool Fleur::Core::BitSet64::ScanFirstFreeForward(uint32_t* val) const
{
    if (m_Bits < 32)
        return bit_scan_forward(static_cast<uint32_t>(~m_Bitmap), val);

    return bit_scan_forward64(~m_Bitmap, val);
}

uint8_t Fleur::Core::BitSet64::MaskCount() const
{
    return m_Bits;
}
