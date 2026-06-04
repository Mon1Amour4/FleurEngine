// Small edge cases for smart pointers: zero-size arrays, containers of Uptr
// surviving reallocation, and the (unsupported) Sptr polymorphic conversion.
// Uses a TU-local live counter checked at TearDown so every test leaves no live
// objects behind.
#include <utility>
#include <vector>

#include "Sptr.h"
#include "Uptr.h"

#include "gtest/gtest.h"

using namespace Fleur;

namespace
{
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
}  // namespace

class SmallEdge : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_Live = 0;
    }
    void TearDown() override
    {
        EXPECT_EQ(g_Live, 0) << "object leak";
    }
};

// --- Zero-size arrays ---
// MakeUnique<int[]>(0): allocate(0) -> ::operator new[](0) returns a valid
// non-null pointer, the construct loop runs zero times, so the Uptr holds a
// non-null pointer with Size()==0. Reset/destruct deallocates that pointer.
// Documents the actual (non-crashing) behavior.
TEST_F(SmallEdge, MakeUniqueArray_ZeroSize_NoCrash)
{
    Uptr<int[]> u = MakeUnique<int[]>(0);
    EXPECT_EQ(u.Size(), 0u);
    // operator new[](0) is allowed to return non-null; we don't assert on the
    // pointer value, only that construction + destruction don't crash.
    SUCCEED();
}

// MakeShared<int[]>(0): same zero-length array path through the shared control
// block. Must construct and destruct cleanly.
TEST_F(SmallEdge, MakeSharedArray_ZeroSize_NoCrash)
{
    Sptr<int[]> s = MakeShared<int[]>(0);
    (void)s;
    SUCCEED();
}

// --- Container of smart pointers surviving reallocation ---
// Push several Uptr<Tracked> into a vector, force a growth reallocation (reserve
// small, then exceed it), and assert values survive the element moves with no
// leak. Uptr is move-only, so the vector must move-construct on reallocation.
TEST_F(SmallEdge, VectorOfUptr_SurvivesReallocation)
{
    std::vector<Uptr<Tracked>> v;
    v.reserve(2);  // small capacity to force a grow

    constexpr int N = 8;
    for (int i = 0; i < N; ++i)
        v.push_back(MakeUnique<Tracked>(i));  // exceeds capacity -> reallocation(s)

    EXPECT_EQ(g_Live, N) << "moves must not create/destroy extra objects";
    for (int i = 0; i < N; ++i)
    {
        ASSERT_TRUE(static_cast<bool>(v[i]));
        EXPECT_EQ(v[i]->value, i) << "value lost across reallocation at " << i;
    }

    v.clear();
    EXPECT_EQ(g_Live, 0);
}

// --- Sptr polymorphic conversion: NOT supported ---
// Sptr has no converting/aliasing ctor (no template <class U> Sptr(const
// Sptr<U>&)), so Sptr<Base> cannot be constructed from Sptr<Derived> and
// polymorphic destruction through a base Sptr is not available. Documented, not
// executed.
TEST_F(SmallEdge, DISABLED_Sptr_PolymorphicConversion_Unsupported)
{
    // struct Base { virtual ~Base() = default; };
    // struct Derived : Base {};
    // Sptr<Base> b = MakeShared<Derived>();  // does NOT compile: no Sptr<U>->Sptr<T> ctor.
    // Add a converting/aliasing constructor to Sptr to enable polymorphic shared
    // ownership, then turn this into a base-deletes-derived counter test.
}
