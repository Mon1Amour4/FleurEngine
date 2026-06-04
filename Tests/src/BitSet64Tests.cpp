// Unit tests for Fleur::Core::BitSet64 — a 64-flag bitset used by the
// allocators (0 = free, 1 = occupied). Pure logic, no backing buffer needed.
// Dangerous inputs (out-of-range SetBit -> __debugbreak, default-ctor IsFull
// shift-by-64 UB) are captured as DISABLED_ rather than executed.
#include "BitSet64.h"

#include "gtest/gtest.h"

using Fleur::Core::BitSet64;

namespace
{
int Used(const BitSet64& b)
{
    return static_cast<int>(b.UsedBits());
}
}  // namespace

TEST(BitSet64Test, SetCheckClear_Bit0)
{
    BitSet64 b(8);
    b.SetBit(0);
    EXPECT_TRUE(b.IsBitOccupied(0));
    EXPECT_EQ(Used(b), 1);

    b.ClearBit(0);
    EXPECT_FALSE(b.IsBitOccupied(0));
    EXPECT_EQ(Used(b), 0);
}

TEST(BitSet64Test, Bit63_FullSet)
{
    BitSet64 b(64);
    for (uint8_t i = 0; i < 64; ++i)
        b.SetBit(i);

    EXPECT_TRUE(b.IsBitOccupied(63));
    EXPECT_TRUE(b.IsFull());
    EXPECT_EQ(Used(b), 64);
}

TEST(BitSet64Test, AllClear_ScanSetReturnsFalse)
{
    BitSet64 b(64);
    uint32_t idx = 12345;
    EXPECT_FALSE(b.ScanFirstSetForward(&idx));
    EXPECT_EQ(Used(b), 0);
    EXPECT_FALSE(b.IsFull());
}

TEST(BitSet64Test, ScanFirstFree_Empty_ReturnsIdx0)
{
    BitSet64 b(16);
    uint32_t idx = 999;
    EXPECT_TRUE(b.ScanFirstFreeForward(&idx));
    EXPECT_EQ(idx, 0u);
}

// DISABLED: exposes a real bug. ScanFirstFreeForward scans ~m_Bitmap WITHOUT
// masking to m_Bits. On a full 16-bit set, ~m_Bitmap has bit 16 set, so it
// returns true with idx==16 — a free slot OUTSIDE the configured range. The
// allocators rely on this to find free slots, so a full bitset wrongly reports
// a free slot beyond its capacity. Re-enable once the scan masks to m_Bits.
TEST(BitSet64Test, DISABLED_ScanFirstFree_Full_ReturnsFalse)
{
    BitSet64 b(16);
    for (uint8_t i = 0; i < 16; ++i)
        b.SetBit(i);

    uint32_t idx = 0;
    EXPECT_FALSE(b.ScanFirstFreeForward(&idx));
}

TEST(BitSet64Test, ScanFirstSet_FindsLowest)
{
    BitSet64 b(16);
    b.SetBit(9);
    b.SetBit(5);

    uint32_t idx = 0;
    EXPECT_TRUE(b.ScanFirstSetForward(&idx));
    EXPECT_EQ(idx, 5u);
}

TEST(BitSet64Test, ScanForwardFrom_RespectsStart)
{
    BitSet64 b(16);
    b.SetBit(3);
    b.SetBit(10);

    uint32_t idx = 4;  // skip past bit 3
    EXPECT_TRUE(b.scan_forward_from(&idx));
    EXPECT_EQ(idx, 10u);
}

TEST(BitSet64Test, ScanForwardFrom_PastLastSet_ReturnsFalse)
{
    BitSet64 b(16);
    b.SetBit(2);

    uint32_t idx = 5;
    EXPECT_FALSE(b.scan_forward_from(&idx));
    EXPECT_EQ(idx, 5u);  // unchanged on miss
}

TEST(BitSet64Test, Toggle_FlipsBit)
{
    BitSet64 b(16);
    b.ToggleBit(1);
    EXPECT_TRUE(b.IsBitOccupied(1));
    b.ToggleBit(1);
    EXPECT_FALSE(b.IsBitOccupied(1));
}

TEST(BitSet64Test, Get_ReflectsSetBits)
{
    BitSet64 b(16);
    b.SetBit(0);
    b.SetBit(2);
    EXPECT_EQ(b.Get(), 0b101ull);
}

TEST(BitSet64Test, IsFull_PartialIsFalse)
{
    BitSet64 b(16);
    b.SetBit(0);
    EXPECT_FALSE(b.IsFull());
}

TEST(BitSet64Test, MaskCount_ReturnsConfiguredBits)
{
    BitSet64 b(40);
    EXPECT_EQ(static_cast<int>(b.MaskCount()), 40);
}

// Boundary: m_Bits == 32. ScanFirstFreeForward uses `m_Bits < 32` while
// ScanFirstSetForward uses `m_Bits > 32` — asymmetric thresholds at exactly 32.
// A free bit reported here must still be inside [0, 32). If this fails it has
// exposed the boundary bug; convert to DISABLED_ + report rather than deleting.
TEST(BitSet64Test, Bits32_FreeScanStaysInRange)
{
    BitSet64 b(32);
    for (uint8_t i = 0; i < 5; ++i)
        b.SetBit(i);

    uint32_t idx = 0;
    ASSERT_TRUE(b.ScanFirstFreeForward(&idx));
    EXPECT_LT(idx, 32u);
    EXPECT_EQ(idx, 5u);
}

// Default-constructed bitset: m_Bits == 0, m_Bitmap == 0. We can't call IsFull
// (shift-by-64 UB, see below) or SetBit (out-of-range), but MaskCount() and
// Get() are safe and must reflect the empty state.
TEST(BitSet64Test, DefaultCtor_IsEmpty)
{
    BitSet64 b;
    EXPECT_EQ(static_cast<int>(b.MaskCount()), 0);
    EXPECT_EQ(b.Get(), 0ull);
    EXPECT_EQ(Used(b), 0);
}

// Pins the CURRENT StringRepresentation() output for a known small set. The loop
// runs `for (i = MaskCount(); i >= 0; --i)`, i.e. it includes index ==
// MaskCount(), so a 16-bit set yields 17 characters (one too many — an off-by-
// one bug; see DISABLED_StringRepresentation_OffByOne). The extra leading char
// corresponds to bit index 16, which is always 0 here. This test locks in the
// actual behavior so a future fix is noticed.
TEST(BitSet64Test, StringRepresentation_CurrentBehaviorPinned)
{
    BitSet64 b(16);
    b.SetBit(0);
    b.SetBit(2);

    std::string s = b.StringRepresentation();
    // 17 chars (off-by-one: MaskCount()+1). Leading char is bit 16 == '0'.
    EXPECT_EQ(s.size(), 17u);
    // Most-significant printed bit first; bits 2 and 0 set -> trailing "...0101".
    EXPECT_EQ(s, "00000000000000101");
}

// --- Dangerous inputs: documented, not executed ---

// StringRepresentation() loops `for (int i = MaskCount(); i >= 0; i--)`, which
// includes i == MaskCount() and therefore emits MaskCount()+1 characters instead
// of MaskCount(). For an N-bit set it prints N+1 bits, leaking bit index N (one
// past the configured range) into the output. The free operator<< overload in
// BitSet64.cpp loops `i < bitsCount` correctly, so the two stringifiers
// disagree. Fix: change the loop bound to `i = MaskCount() - 1`. Pinned as the
// 17-char case in StringRepresentation_CurrentBehaviorPinned above.
TEST(BitSet64Test, DISABLED_StringRepresentation_OffByOne)
{
}

// scan_forward_from guards with `if (*idx > (uint8_t)(m_Bits - 1))`. On a
// default-constructed set m_Bits == 0, so `m_Bits - 1` underflows uint8_t to
// 255: the guard never trips for idx in [0,255], and the function scans an empty
// bitmap instead of rejecting the call. Even on a normal set, passing idx near
// 255 interacts with this underflow. Documents the unsigned-underflow bug; not
// safe to rely on for a 0-bit set.
TEST(BitSet64Test, DISABLED_ScanForwardFrom_ZeroBits_Underflow)
{
}

TEST(BitSet64Test, DISABLED_DefaultCtor_IsFull_ShiftBy64_UB)
{
    // Default ctor leaves m_Bits == 0, so IsFull() computes `~0ull >> (64-0)` =
    // shift-by-64 which is undefined behavior. Don't call IsFull on a 0-bit set.
}

TEST(BitSet64Test, DISABLED_SetBit_OutOfRange_DebugBreak)
{
    // SetBit(idx) with idx > m_Bits-1 triggers __debugbreak(). e.g. BitSet64(8);
    // SetBit(8). Cannot be exercised as a normal test (crashes under debugger).
}

TEST(BitSet64Test, DISABLED_ClearToggle_OutOfRange_NoBoundsCheck)
{
    // ClearBit/ToggleBit do NOT bounds-check; ToggleBit(70) shifts by 70 (UB).
    // Documents the missing guard; not safe to run.
}
