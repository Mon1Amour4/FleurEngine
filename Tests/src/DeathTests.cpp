// Death tests for the assert / __debugbreak abort paths that cannot be exercised
// as normal tests (they crash the process by design). gtest death tests fork a
// child process, run the statement, and assert the child died.
//
// Build context: the MemoryManager lib is compiled with _DEBUG +
// MEMORYMANAGER_PROFILING, so MM_DEBUG_BREAK -> __debugbreak() is live, and
// BitSet64::SetBit out-of-range calls __debugbreak() directly. assert() is also
// active in this Debug build.
//
// IMPORTANT: if any of these prove unreliable on this toolchain (hang, modal
// popup, or fail to trigger), they are converted to DISABLED_ with a note rather
// than left to hang the suite. (This prebuilt gtest does not export
// FLAGS_gtest_death_test_style, so the style is left at its default; on Windows
// gtest spawns the death-test child via CreateProcess regardless.)
#include "BitSet64.h"
#include "MemoryManager.h"

#include "gtest/gtest.h"

using Fleur::Core::BitSet64;

// allocate<int>(0) trips `assert(count > 0 && ...)` in MemoryManager::allocate.
// In a Debug CRT, a failed assert calls abort(), which the death test captures.
TEST(MemoryManagerDeathTest, AllocateZeroCount_Aborts)
{
    // count==0 is a debug-only precondition guarded by assert(), which is compiled
    // out under NDEBUG (Release). There it does NOT abort (allocate(0) just rounds
    // up to a min slot), so the death expectation only holds in a Debug build.
#ifdef NDEBUG
    GTEST_SKIP() << "assert() compiled out with NDEBUG; allocate(0) precondition is debug-only";
#else
    MM::MemoryManager* mm = MM::MemoryManager::ManagerFabric(16ull * 1024 * 1024);
    ASSERT_NE(mm, nullptr);

    // The matcher is "" because the abort/debug-break path produces no stable
    // message on MSVC; we only assert that the child process dies.
    EXPECT_DEATH({ (void)mm->allocate<int>(0); }, "");
#endif
}

// BitSet64(8).SetBit(8): idx (8) > m_Bits - 1 (7) triggers __debugbreak(). With
// no debugger attached the breakpoint exception terminates the child process.
TEST(BitSet64DeathTest, SetBitOutOfRange_DebugBreaks)
{
    EXPECT_DEATH(
        {
            BitSet64 b(8);
            b.SetBit(8);
        },
        "");
}
