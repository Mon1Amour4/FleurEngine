// Edge-case coverage for CoreLib smart pointers, complementing the happy-path
// cases in CoreLibTests.cpp. Focus: observers/const access, comparison
// operators, shared ref-count transitions, self-operations, and reset/release
// robustness. Uses a TU-local live-instance counter (g_Live) checked by the
// fixture so every test must leave no live objects behind.
#include <utility>

#include "Sptr.h"
#include "Uptr.h"
#include "gtest/gtest.h"

using namespace Fleur;

namespace
{
int g_Live = 0;

// Counts live instances; carries a value so we can assert identity/content.
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

// Plain type for arrow/star/const access (does not touch g_Live).
struct Probe
{
    int x = 42;

    int Get() const
    {
        return x;
    }
};
}  // namespace

class CoreLibEdge : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_Live = 0;
    }

    void TearDown() override
    {
        EXPECT_EQ(g_Live, 0);
    }
};

// --- Observers: operator->, operator*, const access ---

TEST_F(CoreLibEdge, Uptr_ArrowAndStar)
{
    Uptr<Probe> p = MakeUnique<Probe>();
    EXPECT_EQ(p->Get(), 42);
    EXPECT_EQ((*p).x, 42);
}

TEST_F(CoreLibEdge, Sptr_ArrowAndStar)
{
    Sptr<Probe> p = MakeShared<Probe>();
    EXPECT_EQ(p->Get(), 42);
    EXPECT_EQ((*p).x, 42);
}

TEST_F(CoreLibEdge, Uptr_ConstAccess)
{
    const Uptr<Probe> p = MakeUnique<Probe>();
    EXPECT_EQ(p.Get()->x, 42);
    EXPECT_EQ(p->Get(), 42);
}

TEST_F(CoreLibEdge, Sptr_ConstAccess)
{
    const Sptr<Probe> p = MakeShared<Probe>();
    EXPECT_EQ(p.Get()->x, 42);
    EXPECT_EQ(p->Get(), 42);
}

// --- Comparison operators ---

TEST_F(CoreLibEdge, Uptr_NullptrComparison)
{
    Uptr<int> empty;
    Uptr<int> full = MakeUnique<int>(1);
    EXPECT_TRUE(empty == nullptr);
    EXPECT_TRUE(nullptr == empty);
    EXPECT_FALSE(full == nullptr);
    EXPECT_TRUE(full != nullptr);
}

TEST_F(CoreLibEdge, Sptr_Comparisons)
{
    Sptr<int> a = MakeShared<int>(1);
    Sptr<int> b = a;                    // same object
    Sptr<int> c = MakeShared<int>(1);   // different object
    Sptr<int> empty;
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(a != c);
    EXPECT_TRUE(empty == nullptr);
    EXPECT_TRUE(a != nullptr);
}

// --- Self-operations (guards must hold) ---

TEST_F(CoreLibEdge, Uptr_SelfMoveAssign)
{
    Uptr<Tracked> p = MakeUnique<Tracked>(7);
    Tracked* raw = p.Get();
    Uptr<Tracked>& ref = p;             // indirection dodges -Wself-move
    p = std::move(ref);
    EXPECT_EQ(p.Get(), raw);            // ownership preserved, no double-free
    EXPECT_EQ(p->value, 7);
}

TEST_F(CoreLibEdge, Sptr_SelfMoveAssign)
{
    Sptr<Tracked> p = MakeShared<Tracked>(7);
    Tracked* raw = p.Get();
    Sptr<Tracked>& ref = p;
    p = std::move(ref);
    EXPECT_EQ(p.Get(), raw);
    EXPECT_EQ(p->value, 7);
}

TEST_F(CoreLibEdge, Uptr_SwapSelf)
{
    Uptr<Tracked> p = MakeUnique<Tracked>(3);
    Tracked* raw = p.Get();
    p.Swap(p);
    EXPECT_EQ(p.Get(), raw);
    EXPECT_EQ(p->value, 3);
}

TEST_F(CoreLibEdge, Uptr_SwapEmptyNonEmpty)
{
    Uptr<Tracked> a = MakeUnique<Tracked>(2);
    Uptr<Tracked> b;
    a.Swap(b);
    EXPECT_FALSE(a);
    EXPECT_TRUE(b);
    EXPECT_EQ(b->value, 2);
}

// --- Reset / Release robustness ---

TEST_F(CoreLibEdge, Uptr_DoubleReset)
{
    Uptr<Tracked> p = MakeUnique<Tracked>(1);
    p.Reset();
    EXPECT_EQ(g_Live, 0);
    p.Reset();                          // second reset on empty must be safe
    EXPECT_FALSE(p);
}

TEST_F(CoreLibEdge, Uptr_ResetAfterRelease)
{
    Uptr<Tracked> p = MakeUnique<Tracked>(1);
    Tracked* raw = p.Release();
    EXPECT_FALSE(p);
    EXPECT_EQ(g_Live, 1);               // released, not destroyed
    p.Reset();                          // no-op on empty, raw stays alive
    EXPECT_EQ(g_Live, 1);
    Uptr<Tracked> adopt(raw);           // re-adopt for proper allocator cleanup
}

TEST_F(CoreLibEdge, Sptr_DoubleReset)
{
    Sptr<Tracked> p = MakeShared<Tracked>(1);
    p.Reset();
    EXPECT_EQ(g_Live, 0);
    p.Reset();
    EXPECT_FALSE(p);
}

// --- Shared ref-count transitions ---

TEST_F(CoreLibEdge, Sptr_TwoOwners_SingleDestruction)
{
    {
        Sptr<Tracked> a = MakeShared<Tracked>(1);
        EXPECT_EQ(a.UseCount(), 1u);
        Sptr<Tracked> b = a;
        EXPECT_EQ(a.UseCount(), 2u);
        EXPECT_EQ(b.UseCount(), 2u);
        EXPECT_EQ(g_Live, 1);           // still one object
    }
    EXPECT_EQ(g_Live, 0);               // destroyed exactly once
}

TEST_F(CoreLibEdge, Sptr_UseCount_DecrementsWhenCopyDestroyed)
{
    Sptr<Tracked> a = MakeShared<Tracked>(1);
    {
        Sptr<Tracked> b = a;
        EXPECT_EQ(a.UseCount(), 2u);
    }
    EXPECT_EQ(a.UseCount(), 1u);
    EXPECT_EQ(g_Live, 1);
}

TEST_F(CoreLibEdge, Sptr_ResetOneOfTwo_KeepsAlive)
{
    Sptr<Tracked> a = MakeShared<Tracked>(5);
    Sptr<Tracked> b = a;
    a.Reset();
    EXPECT_FALSE(a);
    EXPECT_EQ(b.UseCount(), 1u);
    EXPECT_EQ(b->value, 5);
    EXPECT_EQ(g_Live, 1);
}

// --- Weak pointer lifetime ---

TEST_F(CoreLibEdge, Wptr_LockExtendsLifetime)
{
    Sptr<Tracked> a = MakeShared<Tracked>(9);
    Wptr<Tracked> w(a);
    EXPECT_FALSE(w.Expired());

    Sptr<Tracked> locked = w.Lock();
    EXPECT_EQ(a.UseCount(), 2u);

    a.Reset();                          // drop original owner
    EXPECT_FALSE(w.Expired());          // locked copy keeps it alive
    EXPECT_EQ(locked->value, 9);

    locked.Reset();
    EXPECT_TRUE(w.Expired());
    EXPECT_EQ(g_Live, 0);
}

// --- Array Uptr move semantics + factory element construction ---

TEST_F(CoreLibEdge, UptrArray_MoveCtor)
{
    Uptr<Tracked[]> a = MakeUnique<Tracked[]>(3);
    Tracked* raw = a.Get();
    Uptr<Tracked[]> b = std::move(a);
    EXPECT_EQ(b.Get(), raw);
    EXPECT_EQ(a.Get(), nullptr);
    EXPECT_EQ(b.Size(), 3u);
    EXPECT_EQ(g_Live, 3);
}

// DISABLED: exposes a real bug in Uptr<T[]>::operator=(Uptr&&). The pointer is
// moved correctly but the element count is NOT transferred (b.Size() becomes 0),
// so b's destructor frees zero elements -> the moved-in array leaks. Move-ctor
// (UptrArray_MoveCtor above) is fine; only move-assign is broken. Remove the
// DISABLED_ prefix once Uptr<T[]> move-assignment propagates _size.
TEST_F(CoreLibEdge, DISABLED_UptrArray_MoveAssign)
{
    Uptr<Tracked[]> a = MakeUnique<Tracked[]>(2);
    Tracked* raw = a.Get();
    Uptr<Tracked[]> b = MakeUnique<Tracked[]>(4);
    b = std::move(a);
    EXPECT_EQ(b.Get(), raw);
    EXPECT_EQ(b.Size(), 2u);
    EXPECT_EQ(a.Get(), nullptr);
    EXPECT_EQ(g_Live, 2);               // the 4-element array was freed
}

TEST_F(CoreLibEdge, MakeUniqueArray_ConstructsAllElements)
{
    Uptr<Tracked[]> a = MakeUnique<Tracked[]>(4);
    EXPECT_EQ(a.Size(), 4u);
    EXPECT_EQ(g_Live, 4);
}

// ===========================================================================
// TODO stubs — remaining coverage gaps. Bodies are intentionally unwritten:
// fill in Arrange/Act/Assert and drop the GTEST_SKIP. Grouped by theme.
// ===========================================================================

// --- Bug-exposing (KEEP skipped until the source bug is fixed; un-skipping now
//     would crash or fail the runner). See the bug report. ---

TEST_F(CoreLibEdge, SptrArray_GetOnEmpty_NoNullDeref)
{
    // Arrange: a default-constructed (or moved-from / Reset) Sptr<Tracked[]>.
    // Act: call Get() on it.
    // Assert: returns nullptr without dereferencing a null control block.
    // BUG: Sptr<T[]>::Get() const does `return m_Cb->_ptr;` with no null guard
    //      (single-Sptr has one) -> null deref crash on empty array Sptr.
    GTEST_SKIP() << "TODO: implement after fixing Sptr<T[]>::Get null guard";
}

TEST_F(CoreLibEdge, WptrArray_OutlivesSptr_FreesControlBlock)
{
    // Arrange: Sptr<Tracked[]> in inner scope, a Wptr<Tracked[]> observing it.
    // Act: let the Sptr die, then let the Wptr (last ref) destroy.
    // Assert: control block is freed cleanly; no leak (g_Live == 0), no crash.
    // BUG: Wptr<T[]>::_alloc is defaulted null and never assigned, then
    //      _alloc->deallocate() runs when the last weak ref frees the CB -> crash.
    GTEST_SKIP() << "TODO: implement after fixing Wptr<T[]>::_alloc init";
}

TEST_F(CoreLibEdge, MakeUnique_ThrowingCtor_NoLeak)
{
    // Arrange: a type whose constructor throws after N successful constructions.
    // Act: MakeUnique<Throwing>() / MakeUnique<Throwing[]>(k) inside EXPECT_THROW.
    // Assert: every already-constructed element is destroyed and the raw block is
    //         freed -> g_Live == 0 (no leak). Verify the array catch block frees
    //         the right size (size * sizeof(T), not sizeof(T)).
    GTEST_SKIP() << "TODO: implement (exception safety of factory)";
}

TEST_F(CoreLibEdge, MakeShared_ThrowingCtor_NoLeak)
{
    // Same as above for MakeShared / MakeShared<T[]>. Exposes the array catch-block
    // deallocate-size mismatch (Sptr.h MakeShared<T[]> uses sizeof(T)).
    GTEST_SKIP() << "TODO: implement (exception safety of factory)";
}

TEST_F(CoreLibEdge, SptrArray_DefaultAndMovedFrom_SizeIsZero)
{
    // Arrange: default-constructed Sptr<Tracked[]>, and a moved-from one.
    // Act: read GetSize().
    // Assert: size is a defined 0, not garbage.
    // BUG: Sptr<T[]>::_size member has no initializer -> indeterminate value.
    GTEST_SKIP() << "TODO: implement after zero-initializing Sptr<T[]>::_size";
}

// --- Weak pointer (single) remaining ops ---

TEST_F(CoreLibEdge, Wptr_MoveAssign)
{
    // Arrange: w1 observing object A, w2 observing object B (or empty).
    // Act: w2 = std::move(w1); also test self-move (w = std::move(w)).
    // Assert: w2 observes A (not Expired, Lock yields A); w1 left empty/Expired.
    GTEST_SKIP() << "TODO: implement";
}

TEST_F(CoreLibEdge, Wptr_AssignFromSptr)
{
    // Arrange: a live Sptr and a Wptr observing something else.
    // Act: wptr = sptr; (operator= from Sptr).
    // Assert: wptr now observes the Sptr's object; Expired() false; Lock matches.
    GTEST_SKIP() << "TODO: implement";
}

TEST_F(CoreLibEdge, Wptr_Swap)
{
    // Arrange: two Wptrs observing different objects (+ empty variants).
    // Act: w1.Swap(w2); plus swap-with-self.
    // Assert: observed objects exchanged; Expired states exchanged; self-swap noop.
    GTEST_SKIP() << "TODO: implement";
}

// --- Shared array (Sptr<T[]>) ownership ops (only ctor/Reset/[] are covered) ---

TEST_F(CoreLibEdge, SptrArray_CopyMoveAssignSwap_RefCount)
{
    // Arrange: Sptr<Tracked[]> of size N via MakeShared<Tracked[]>(N).
    // Act: copy, move, copy-assign, move-assign, Swap between instances.
    // Assert: UseCount tracks owners; GetSize preserved through move/swap;
    //         operator bool correct; all elements destroyed once at the end
    //         (g_Live == 0). One test per op is fine.
    GTEST_SKIP() << "TODO: implement (array Sptr is largely untested)";
}

TEST_F(CoreLibEdge, SptrArray_TwoOwners_SingleDestructionOfAllElements)
{
    // Arrange: array Sptr of N Tracked, copy it (UseCount 2).
    // Act: drop both owners.
    // Assert: each of the N elements destroyed exactly once (g_Live 0), no double free.
    GTEST_SKIP() << "TODO: implement";
}

// --- Misc owning-pointer gaps ---

TEST_F(CoreLibEdge, Sptr_DtorReleasesLastOwner)
{
    // Arrange: single Sptr<Tracked> in a scope (no Reset used).
    // Act: leave the scope so the destructor runs the Release() path.
    // Assert: object destroyed (g_Live 0). Complements the Reset()-based leak tests.
    GTEST_SKIP() << "TODO: implement";
}

TEST_F(CoreLibEdge, Sptr_SelfCopyAssign)
{
    // Arrange: Sptr<Tracked> p with UseCount 1.
    // Act: p = p; through a reference (dodges -Wself-assign).
    // Assert: object intact, UseCount still 1, no double-free / no leak.
    GTEST_SKIP() << "TODO: implement";
}

TEST_F(CoreLibEdge, Sptr_SwapSelfAndEmpty)
{
    // Arrange: non-empty and empty Sptr.
    // Act: a.Swap(a) (self) and a.Swap(empty).
    // Assert: self-swap is a noop; empty<->non-empty exchanges ownership cleanly.
    GTEST_SKIP() << "TODO: implement";
}

TEST_F(CoreLibEdge, Uptr_PointerArithmetic_Minus)
{
    // Arrange: array Uptr; take element pointers.
    // Act: operator-(Uptr, Uptr) and operator-(Uptr, ptrdiff_t).
    // Assert: differences/offsets match raw pointer arithmetic. (operator+ already
    //         covered by UptrArray_PointerArithmetic in CoreLibTests.cpp.)
    GTEST_SKIP() << "TODO: implement";
}

TEST_F(CoreLibEdge, MakeArray_ForwardsArgsToEachElement)
{
    // Arrange: pick a value v.
    // Act: MakeUnique<Tracked[]>(N, v) and MakeShared<Tracked[]>(N, v).
    // Assert: every element has value == v (verifies args are forwarded to each
    //         element ctor, not just default-constructed). If only the first/none
    //         gets v, that's a forwarding bug worth flagging.
    GTEST_SKIP() << "TODO: implement";
}

TEST_F(CoreLibEdge, MovedFrom_IsNullAndReusable)
{
    // Arrange: Uptr / Sptr / Wptr, then std::move out of each.
    // Act: inspect the moved-from object, then reassign / Reset it.
    // Assert: moved-from is empty/null/Expired and safe to reuse and to destroy.
    GTEST_SKIP() << "TODO: implement";
}

// --- Custom allocator path (default DefaultAllocator is the only one exercised) ---

TEST_F(CoreLibEdge, CustomAllocator_AllocateDeallocatePaired)
{
    // Arrange: a counting allocator satisfying FleurAllocator (tracks bytes/calls),
    //          threaded through Uptr<int, CountingAlloc> / Sptr<int, CountingAlloc>
    //          and their array forms.
    // Act: construct via MakeUnique/MakeShared with that allocator, then destroy.
    // Assert: every allocate is matched by exactly one deallocate, with the SAME
    //         size argument. This both documents the custom-allocator path and
    //         catches the array deallocate-size mismatches (sizeof(T) vs N*sizeof(T)).
    GTEST_SKIP() << "TODO: implement counting allocator + wire through";
}
