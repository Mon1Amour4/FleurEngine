// Unit tests for PageAllocator — a bump allocator of fixed-size pages with a
// front-page rewind path and a cached free-list for non-front frees. Tested in
// isolation over a heap-backed buffer.
#include <vector>

#include "PageAllocator.hpp"

#include "gtest/gtest.h"

namespace
{
constexpr uint32_t kPage = 4096;
}  // namespace

TEST(PageAllocatorTest, AllocateSequential_PointersPageApart)
{
    std::vector<unsigned char> buf(4 * kPage);
    PageAllocator pa(buf.data(), buf.size(), kPage);

    unsigned char* p1 = pa.allocate_page();
    unsigned char* p2 = pa.allocate_page();
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    EXPECT_EQ(static_cast<size_t>(p2 - p1), static_cast<size_t>(kPage));
}

TEST(PageAllocatorTest, CapacityExhausted_ReturnsNull)
{
    std::vector<unsigned char> buf(2 * kPage);
    PageAllocator pa(buf.data(), buf.size(), kPage);

    EXPECT_NE(pa.allocate_page(), nullptr);
    EXPECT_NE(pa.allocate_page(), nullptr);
    EXPECT_EQ(pa.allocate_page(), nullptr);  // third exceeds capacity
}

TEST(PageAllocatorTest, FreeFrontPage_Rewinds_ReusesAddress)
{
    std::vector<unsigned char> buf(4 * kPage);
    PageAllocator pa(buf.data(), buf.size(), kPage);

    unsigned char* p1 = pa.allocate_page();
    pa.free(p1);                              // p1 is the front page -> rewind
    unsigned char* p2 = pa.allocate_page();
    EXPECT_EQ(p1, p2);
}

TEST(PageAllocatorTest, FreeNonFrontPage_Cached_Reused)
{
    std::vector<unsigned char> buf(4 * kPage);
    PageAllocator pa(buf.data(), buf.size(), kPage);

    unsigned char* p1 = pa.allocate_page();
    pa.allocate_page();                       // p2
    pa.allocate_page();                       // p3 (front)

    pa.free(p1);                              // non-front -> goes to cache list
    unsigned char* reused = pa.allocate_page();
    EXPECT_EQ(reused, p1);                    // cache served before bump
}

TEST(PageAllocatorTest, FreeWhenEmpty_NoOp)
{
    std::vector<unsigned char> buf(2 * kPage);
    PageAllocator pa(buf.data(), buf.size(), kPage);

    pa.free(buf.data());                      // nothing allocated -> guarded no-op
    unsigned char* p = pa.allocate_page();
    EXPECT_EQ(p, buf.data());                 // bump pointer untouched
}

TEST(PageAllocatorTest, AllocatePagesSize_RoundsUpAndReportsCount)
{
    std::vector<unsigned char> buf(8 * kPage);
    PageAllocator pa(buf.data(), buf.size(), kPage);

    uint32_t count = 0;
    unsigned char* p = pa.allocate_pages_size(kPage + kPage / 2, &count);  // 1.5 pages
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(count, 2u);
}

TEST(PageAllocatorTest, AllocatePagesN_ContiguousBlock)
{
    std::vector<unsigned char> buf(8 * kPage);
    PageAllocator pa(buf.data(), buf.size(), kPage);

    unsigned char* block = pa.allocate_pages_n(3);
    unsigned char* next = pa.allocate_page();
    ASSERT_NE(block, nullptr);
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(static_cast<size_t>(next - block), static_cast<size_t>(3 * kPage));
}

// Smoke check that the FleurTestsAsan build actually instruments code: run that
// build with --gtest_also_run_disabled_tests and AddressSanitizer must report a
// heap-buffer-overflow here. DISABLED so it never runs in the normal build.
TEST(PageAllocatorTest, DISABLED_AsanSmoke_HeapOverflow)
{
    volatile int* p = new int[4];
    p[5] = 42;  // intentional out-of-bounds write
    delete[] p;
}
