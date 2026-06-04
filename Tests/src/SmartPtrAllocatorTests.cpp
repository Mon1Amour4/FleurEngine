// Custom-allocator path for Uptr / Sptr.
//
// The engine's MM-backed allocator (Fleur::Memory::FleurAllocator in
// CoreLib/FleurAllocator.hpp) does NOT satisfy the FleurAllocator *concept* used
// by Uptr/Sptr: the concept (CoreLibConcepts.h) requires byte-based
//   allocate(size_t)  -> convertible_to<uint8_t*>
//   deallocate(uint8_t*, size_t)
// whereas Memory::FleurAllocator is a typed allocator (allocate(uint32_t count)
// -> value_type*). It also needs a live MemoryManager wired through a singleton
// (AllocAdapter::Init), with no zero-arg way to obtain one in a unit test.
//
// So, per the task, we exercise the custom-allocator path with a minimal byte
// allocator that satisfies the concept and forwards to operator new[]/delete[],
// while counting allocate/deallocate calls. This proves Uptr/Sptr thread a
// user-supplied allocator and pair allocate/deallocate correctly. The
// MM-backed-allocator-through-the-singleton path is left out (see the DISABLED
// note at the bottom) because it requires engine plumbing.
#include <cstdint>

#include "CoreLibConcepts.h"
#include "Sptr.h"
#include "Uptr.h"

#include "gtest/gtest.h"

using namespace Fleur;

namespace
{
// Counts live instances so leaks surface as a non-zero g_Live at TearDown.
int g_Live = 0;

struct Tracked
{
    int value = 0;

    Tracked()
    {
        ++g_Live;
    }
    explicit Tracked(int v) : value(v)
    {
        ++g_Live;
    }
    ~Tracked()
    {
        --g_Live;
    }
};

// Minimal byte allocator satisfying the FleurAllocator concept. Counts paired
// allocate/deallocate calls via static counters (Uptr/Sptr each hold a value
// copy, so per-instance state would not survive moves; statics track the TU).
struct CountingAllocator
{
    static int allocs;
    static int deallocs;

    CountingAllocator() = default;

    uint8_t* allocate(size_t size) noexcept
    {
        ++allocs;
        return static_cast<uint8_t*>(::operator new[](size, std::nothrow));
    }

    void deallocate(uint8_t* ptr, size_t /*size*/) noexcept
    {
        ++deallocs;
        ::operator delete[](ptr, std::nothrow);
    }
};

int CountingAllocator::allocs = 0;
int CountingAllocator::deallocs = 0;
}  // namespace

// Concept self-check: the custom allocator must actually satisfy FleurAllocator,
// otherwise Uptr<int, CountingAllocator> would not compile for the right reason.
static_assert(FleurAllocator<CountingAllocator>, "CountingAllocator must satisfy the FleurAllocator concept");

class SmartPtrAllocator : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_Live = 0;
        CountingAllocator::allocs = 0;
        CountingAllocator::deallocs = 0;
    }

    void TearDown() override
    {
        EXPECT_EQ(g_Live, 0) << "object leak";
        EXPECT_EQ(CountingAllocator::allocs, CountingAllocator::deallocs) << "allocate/deallocate not paired";
    }
};

// --- Uptr with custom allocator: construct / use / reset round-trip ---

TEST_F(SmartPtrAllocator, Uptr_CustomAllocator_ConstructUseReset)
{
    Uptr<Tracked, CountingAllocator> p = MakeUnique<Tracked, CountingAllocator>(7);
    ASSERT_TRUE(static_cast<bool>(p));
    EXPECT_EQ(p->value, 7);
    EXPECT_EQ(g_Live, 1);
    EXPECT_EQ(CountingAllocator::allocs, 1);

    p.Reset();  // destroys + deallocates through the custom allocator
    EXPECT_FALSE(static_cast<bool>(p));
    EXPECT_EQ(g_Live, 0);
    EXPECT_EQ(CountingAllocator::deallocs, 1);
}

// --- Uptr move keeps a single allocate/deallocate pair ---

TEST_F(SmartPtrAllocator, Uptr_CustomAllocator_MoveKeepsPairing)
{
    Uptr<Tracked, CountingAllocator> a = MakeUnique<Tracked, CountingAllocator>(3);
    Uptr<Tracked, CountingAllocator> b = std::move(a);  // no extra alloc on move

    EXPECT_FALSE(static_cast<bool>(a));
    ASSERT_TRUE(static_cast<bool>(b));
    EXPECT_EQ(b->value, 3);
    EXPECT_EQ(CountingAllocator::allocs, 1) << "move must not allocate";
    EXPECT_EQ(g_Live, 1);
}

// --- Sptr with custom allocator: shared use + counted destruction ---
// Canonical Sptr round-trip through the custom allocator: MakeShared allocates
// both the control block and the object via CountingAllocator, copies share the
// count, and the last destruction frees everything.
TEST_F(SmartPtrAllocator, Sptr_CustomAllocator_MakeSharedRoundTrip)
{
    {
        Sptr<Tracked, CountingAllocator> a = MakeShared<Tracked, CountingAllocator>(11);
        ASSERT_EQ(a->value, 11);
        EXPECT_EQ(g_Live, 1);

        Sptr<Tracked, CountingAllocator> b = a;  // shares ownership
        EXPECT_EQ(a.UseCount(), 2u);
        EXPECT_EQ(g_Live, 1) << "copy must not create a second object";
    }
    EXPECT_EQ(g_Live, 0) << "last Sptr out of scope must destroy the object";
}

// MM-backed allocator through the engine singleton is out of scope for a unit
// test: Memory::FleurAllocator doesn't satisfy the byte-allocator concept AND
// needs AllocAdapter::Init(live MemoryManager). Documented, not implemented.
TEST_F(SmartPtrAllocator, DISABLED_MMBackedAllocator_NeedsEnginePlumbing)
{
    // Wiring Uptr<int, Memory::FleurAllocator> requires (a) adapting the typed
    // allocate(count)->T* API to the byte allocate(size)->uint8_t* concept and
    // (b) a process-wide AllocAdapter initialized with a live MemoryManager.
    // Both are engine-integration concerns, not unit-testable in isolation.
}
