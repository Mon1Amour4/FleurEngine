// Unit tests for MM::MemoryManager — the top-level allocator that routes by
// size: <= SMALL_SIZE (2032) -> SLUB, (2032, 4 MiB) -> TLSF, >= 4 MiB unsupported.
// Benchmark/stress cases live in Tests/benchmarks/MemoryManagerBench.cpp.
//
// Notes baked into these tests (from the current implementation):
//  - The ctor is private; the only way to create a manager is ManagerFabric().
//  - There is no public used-bytes accessor and Print() is a no-op; GetSnapshot()
//    returns a string. State is asserted via pointer behaviour, not counters.
//  - deallocate<T>() does NOT call destructors (see DISABLED_Deallocate_RunsDestructors).
//  - The Align template parameter is plumbed but ignored (over-alignment unsupported).
#include <cstdint>

#include "MemoryManager.h"

#include "gtest/gtest.h"

namespace
{
// Counts live instances so construct_at / construct_array_at can be verified.
struct CtorCounter
{
    static int alive;
    int v = 0;

    CtorCounter()
    {
        ++alive;
    }

    explicit CtorCounter(int x) : v(x)
    {
        ++alive;
    }

    ~CtorCounter()
    {
        --alive;
    }
};

int CtorCounter::alive = 0;
}  // namespace

class MemoryManagerTest : public ::testing::Test
{
protected:
    // 16 MiB is far below the old 5 GB config but large enough for SLUB/TLSF init
    // plus chunk/page growth in these tests.
    static constexpr size_t kCapacity = 16ull * 1024 * 1024;

    MM::MemoryManager* mm = nullptr;

    void SetUp() override
    {
        mm = MM::MemoryManager::ManagerFabric(kCapacity);
        ASSERT_NE(mm, nullptr);
    }

    // No TearDown: ~MemoryManager is a no-op and the backing VirtualAlloc region
    // is reclaimed at process exit. Deleting mm would corrupt the heap (it lives
    // inside that region). Each test gets a fresh manager.
};

// --- Revived from the old disabled set, fixed for the current API ---

TEST_F(MemoryManagerTest, AllocateStoresData)
{
    int* ptr = mm->allocate<int>(40);
    ASSERT_NE(ptr, nullptr);

    for (int i = 0; i < 40; ++i)
        ptr[i] = i;
    for (int i = 0; i < 40; ++i)
        EXPECT_EQ(ptr[i], i) << "data corrupted at index " << i;

    mm->deallocate<int>(ptr, 40);
}

TEST_F(MemoryManagerTest, MultipleAllocations_NonNullDistinct)
{
    int* p1 = mm->allocate<int>(10);
    float* p2 = mm->allocate<float>(20);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    EXPECT_NE(static_cast<void*>(p1), static_cast<void*>(p2));
    // Address ordering is intentionally NOT asserted: different size classes live
    // in different pools, so layout order does not track allocation order.
}

TEST_F(MemoryManagerTest, Alignment_Double)
{
    double* ptr = mm->allocate<double>(1);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % alignof(double), 0u);
}

// --- SLUB range (<= 2032 bytes) ---

TEST_F(MemoryManagerTest, DistinctAllocations_NonOverlapping)
{
    int* a = mm->allocate<int>(10);
    int* b = mm->allocate<int>(10);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    auto a0 = reinterpret_cast<uintptr_t>(a);
    auto b0 = reinterpret_cast<uintptr_t>(b);
    uintptr_t a1 = a0 + 10 * sizeof(int);
    uintptr_t b1 = b0 + 10 * sizeof(int);
    EXPECT_TRUE(a1 <= b0 || b1 <= a0) << "live allocations overlap";
}

TEST_F(MemoryManagerTest, FreeThenAlloc_SameSize_ReusesAddress)
{
    int* a = mm->allocate<int>(4);
    ASSERT_NE(a, nullptr);
    mm->deallocate<int>(a, 4);

    int* b = mm->allocate<int>(4);
    EXPECT_EQ(a, b) << "freed slot should become the free-list head and be reused";
}

TEST_F(MemoryManagerTest, DeallocateNull_NoOp)
{
    mm->deallocate<int>(nullptr, 4);  // guarded by `if (!ptr) return;`
    SUCCEED();
}

TEST_F(MemoryManagerTest, Boundary_2032_RoutesToSlub)
{
    struct Big
    {
        uint8_t d[2032];
    };

    Big* p = mm->allocate<Big>(1);  // == SMALL_SIZE, largest SLUB slot
    ASSERT_NE(p, nullptr);
    p->d[0] = 1;
    p->d[2031] = 2;
    EXPECT_EQ(p->d[0], 1);
    EXPECT_EQ(p->d[2031], 2);
    mm->deallocate<Big>(p, 1);
}

TEST_F(MemoryManagerTest, Boundary_MinSlot_RoundsUp)
{
    char* p = mm->allocate<char>(1);  // 1 byte -> rounded up to 16-byte slot
    ASSERT_NE(p, nullptr);
    *p = 'x';
    EXPECT_EQ(*p, 'x');
    mm->deallocate<char>(p, 1);
}

TEST_F(MemoryManagerTest, ManySmallAllocs_GrowsChunks_NoAliasing)
{
    constexpr int N = 200;  // 200 * 256B forces multiple chunks per pool
    int* ptrs[N];
    for (int i = 0; i < N; ++i)
    {
        ptrs[i] = mm->allocate<int>(64);
        ASSERT_NE(ptrs[i], nullptr) << "failed at allocation " << i;
        ptrs[i][0] = i;
    }
    for (int i = 0; i < N; ++i)
        EXPECT_EQ(ptrs[i][0], i) << "aliasing/corruption at " << i;
    for (int i = 0; i < N; ++i)
        mm->deallocate<int>(ptrs[i], 64);
}

// --- TLSF range (2033 .. ~4 MiB) ---

TEST_F(MemoryManagerTest, Boundary_2033_RoutesToTlsf)
{
    struct T2033
    {
        uint8_t d[2033];
    };

    T2033* p = mm->allocate<T2033>(1);  // SMALL_SIZE + 1 -> first TLSF size
    ASSERT_NE(p, nullptr);
    p->d[0] = 7;
    p->d[2032] = 9;
    EXPECT_EQ(p->d[0], 7);
    EXPECT_EQ(p->d[2032], 9);
    mm->deallocate<T2033>(p, 1);
}

TEST_F(MemoryManagerTest, Tlsf_LargeAlloc_StoresData)
{
    constexpr int N = 16 * 1024;  // 64 KiB of ints -> TLSF
    int* p = mm->allocate<int>(N);
    ASSERT_NE(p, nullptr);
    for (int i = 0; i < N; ++i)
        p[i] = i;
    for (int i = 0; i < N; ++i)
        ASSERT_EQ(p[i], i);
    mm->deallocate<int>(p, N);
}

TEST_F(MemoryManagerTest, Tlsf_FreeAndRealloc_NonNull)
{
    constexpr int N = 8 * 1024;  // 32 KiB
    int* a = mm->allocate<int>(N);
    ASSERT_NE(a, nullptr);
    mm->deallocate<int>(a, N);

    int* b = mm->allocate<int>(N);  // exercises split/merge of freed block
    EXPECT_NE(b, nullptr);
    mm->deallocate<int>(b, N);
}

// --- construct_at / construct_array_at ---

TEST_F(MemoryManagerTest, ConstructAt_RunsConstructor)
{
    CtorCounter::alive = 0;

    CtorCounter* p = mm->construct_at<CtorCounter>(42);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(CtorCounter::alive, 1);
    EXPECT_EQ(p->v, 42);

    p->~CtorCounter();              // deallocate doesn't destruct -> do it manually
    mm->deallocate<CtorCounter>(p, 1);
    EXPECT_EQ(CtorCounter::alive, 0);
}

TEST_F(MemoryManagerTest, ConstructArrayAt_RunsNConstructors)
{
    CtorCounter::alive = 0;
    constexpr uint32_t N = 5;

    CtorCounter* p = mm->construct_array_at<CtorCounter>(N, 7);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(CtorCounter::alive, static_cast<int>(N));
    for (uint32_t i = 0; i < N; ++i)
        EXPECT_EQ(p[i].v, 7);

    for (uint32_t i = 0; i < N; ++i)
        p[i].~CtorCounter();
    mm->deallocate<CtorCounter>(p, N);
    EXPECT_EQ(CtorCounter::alive, 0);
}

// --- Dangerous / unsupported paths: documented, not executed ---

TEST_F(MemoryManagerTest, DISABLED_Deallocate_RunsDestructors)
{
    // Documents a real gap: deallocate<T>() frees the slot but never calls ~T().
    // Enable once deallocate invokes destructors for single + array allocations.
}

TEST_F(MemoryManagerTest, DISABLED_AllocateAtMediumSize_Unsupported)
{
    // size >= MEDIUN_SIZE (4 MiB) hits MM_DEBUG_BREAK(true): __debugbreak() in
    // debug+profiling builds (crash), nullptr otherwise. Unsupported large path.
}

TEST_F(MemoryManagerTest, DISABLED_OverAlignment_NotHonored)
{
    // allocate<T, 64>(1): the Align template parameter is ignored by both SLUB
    // and TLSF, so 64-byte alignment is not guaranteed. Enable once honored.
}

TEST_F(MemoryManagerTest, DISABLED_CountZero_Asserts)
{
    // allocate<int>(0) trips assert(count > 0). Death-test territory only.
}
