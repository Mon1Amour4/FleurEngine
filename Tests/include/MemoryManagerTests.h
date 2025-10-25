#pragma once

#include <random>

#include "MemoryManager.h"
#include "SyntheticTypes.hpp"
#include "gtest/gtest.h"

#define TEST_SUITE_NAME MemoryManagerTest
#define ManagerConfig 1024ULL * 1024ULL * 1024ULL * 5ULL, 1024ULL * 1024ULL * 1024ULL * 2ULL, MM::PAGE_SIZE, MM::MIN_SLOT_SIZE

void RangedTest(uint32_t from, uint32_t to)
{
    MM::MemoryManager manager(ManagerConfig);
    size_t allocated = 0;
    uint32_t allocationsCupBytes = 1024 * 1024 * 1024;

    MM::Benchmark mark{2};

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> actionDist(from, to);
    std::vector<std::pair<int, int*>> mm_pairs;
    std::vector<std::pair<int, int*>> std_pairs;
    std::cout << "Memory Manager benchmark\n";
    for (size_t i = 0; allocated < allocationsCupBytes; i++)
    {
        int size = (actionDist(gen) / sizeof(int) * sizeof(int));
        int count = size / sizeof(int);
        allocated += size;

        mark.StartAlloc();
        mm_pairs.push_back({count, manager.allocate<int>(count)});
        mark.EndAlloc();
    }

    for (auto alloc : mm_pairs)
    {
        mark.StartDealloc();
        manager.deallocate<int>(alloc.second, alloc.first);
        mark.EndDealloc();
    }
    mark.Print();

    std::allocator<int> alloc;
    std::cout << "STD allocator benchmark\n";
    MM::Benchmark std_mark{2};
    for (size_t i = 0; i < mm_pairs.size(); i++)
    {
        std_mark.StartAlloc();
        uint32_t count = mm_pairs[i].first;
        std_pairs.push_back({count, alloc.allocate(count)});
        std_mark.EndAlloc();
    }
    for (size_t i = 0; i < mm_pairs.size(); i++)
    {
        std_mark.StartDealloc();
        alloc.deallocate(std_pairs[i].second, std_pairs[i].first);
        std_mark.EndDealloc();
    }
    std_mark.Print();
}

#if 0 Random Allocations
TEST(TEST_SUITE_NAME, RandomAllocations)
{
    using namespace MM;
    MemoryManager manager(ManagerConfig);
    manager.ClearFile("MemorySnapshot.txt");

    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> actionDist(0, 1);
    std::uniform_int_distribution<> countDist(1, 10);
    std::uniform_int_distribution<> typeDist(0, 4);

    struct AllocRecord
    {
        uint8_t type;
        uint8_t count;
        uint32_t id;
        void* ptr;
        bool alloc;
    };

    std::vector<AllocRecord> allocated;
    std::vector<AllocRecord> allocated_std;
    std::vector<AllocRecord> alloc_free;

    allocated.reserve(1000000);
    allocated_std.reserve(1000000);
    alloc_free.reserve(1000000);

    uint32_t idCounter = 0;

    auto allocFuncs = std::to_array({
        +[](MemoryManager& m, int c) -> void* { return m.allocate<Synthetic::Synthetic1>(c); },
        +[](MemoryManager& m, int c) -> void* { return m.allocate<Synthetic::Synthetic2>(c); },
        +[](MemoryManager& m, int c) -> void* { return m.allocate<Synthetic::Synthetic3>(c); },
        +[](MemoryManager& m, int c) -> void* { return m.allocate<Synthetic::Synthetic4>(c); },
        +[](MemoryManager& m, int c) -> void* { return m.allocate<Synthetic::Synthetic5>(c); },
    });

    auto deallocFuncs = std::to_array({
        +[](MemoryManager& m, void* p, int c) { m.deallocate<Synthetic::Synthetic1>((Synthetic::Synthetic1*)p, c); },
        +[](MemoryManager& m, void* p, int c) { m.deallocate<Synthetic::Synthetic2>((Synthetic::Synthetic2*)p, c); },
        +[](MemoryManager& m, void* p, int c) { m.deallocate<Synthetic::Synthetic3>((Synthetic::Synthetic3*)p, c); },
        +[](MemoryManager& m, void* p, int c) { m.deallocate<Synthetic::Synthetic4>((Synthetic::Synthetic4*)p, c); },
        +[](MemoryManager& m, void* p, int c) { m.deallocate<Synthetic::Synthetic5>((Synthetic::Synthetic5*)p, c); },
    });

    auto allocStdFuncs = std::to_array({
        +[](int c) -> void*
        {
            std::allocator<Synthetic::Synthetic1> a;
            return a.allocate(c);
        },
        +[](int c) -> void*
        {
            std::allocator<Synthetic::Synthetic2> a;
            return a.allocate(c);
        },
        +[](int c) -> void*
        {
            std::allocator<Synthetic::Synthetic3> a;
            return a.allocate(c);
        },
        +[](int c) -> void*
        {
            std::allocator<Synthetic::Synthetic4> a;
            return a.allocate(c);
        },
        +[](int c) -> void*
        {
            std::allocator<Synthetic::Synthetic5> a;
            return a.allocate(c);
        },
    });

    auto deallocStdFuncs = std::to_array({
        +[](void* p, int c)
        {
            std::allocator<Synthetic::Synthetic1> a;
            a.deallocate((Synthetic::Synthetic1*)p, c);
        },
        +[](void* p, int c)
        {
            std::allocator<Synthetic::Synthetic2> a;
            a.deallocate((Synthetic::Synthetic2*)p, c);
        },
        +[](void* p, int c)
        {
            std::allocator<Synthetic::Synthetic3> a;
            a.deallocate((Synthetic::Synthetic3*)p, c);
        },
        +[](void* p, int c)
        {
            std::allocator<Synthetic::Synthetic4> a;
            a.deallocate((Synthetic::Synthetic4*)p, c);
        },
        +[](void* p, int c)
        {
            std::allocator<Synthetic::Synthetic5> a;
            a.deallocate((Synthetic::Synthetic5*)p, c);
        },
    });

    Benchmark mark{2};

    constexpr size_t iterations = 10'000'000;
    for (size_t i = 0; i < iterations; ++i)
    {
        bool doFree = (actionDist(gen) == 1);
        int count = countDist(gen);
        int type = typeDist(gen);

        if (doFree && !allocated.empty())
        {
            std::uniform_int_distribution<size_t> freeIndexDist(0, allocated.size() - 1);
            size_t idx = freeIndexDist(gen);
            auto& rec = allocated[idx];

            mark.StartDealloc();
            deallocFuncs[rec.type](manager, rec.ptr, rec.count);
            mark.EndDealloc();

            alloc_free.push_back({(uint8_t)rec.type, (uint8_t)rec.count, rec.id, rec.ptr, false});


            allocated[idx] = allocated.back();
            allocated.pop_back();
        }
        else
        {
            size_t size = Synthetic::SizeOf(type) * count;
            if (size > 2048)
                continue;

            ++idCounter;

            mark.StartAlloc();
            void* ptr = allocFuncs[type](manager, count);
            mark.EndAlloc();

            allocated.push_back({(uint8_t)type, (uint8_t)count, idCounter, ptr, true});
            allocated_std.push_back({(uint8_t)type, (uint8_t)count, idCounter, ptr, true});
            alloc_free.push_back({(uint8_t)type, (uint8_t)count, idCounter, ptr, true});
        }
    }

    mark.Print();

    Benchmark std_mark{2};
    std::unordered_map<uint64_t, void*> helper;
    for (auto rec : alloc_free)
    {
        if (rec.alloc)
        {
            std_mark.StartAlloc();
            helper[rec.id] = allocStdFuncs[rec.type](rec.count);
            std_mark.EndAlloc();
        }
        else
        {
            auto it = helper.find(rec.id);
            if (it != helper.end())
            {
                std_mark.StartDealloc();
                deallocStdFuncs[rec.type](it->second, rec.count);
                std_mark.EndDealloc();

                helper.erase(it);
            }
        }
    }

    std_mark.Print();
}
#endif

#if 1 FixedRangeAllocationsFrom64To128
TEST(TEST_SUITE_NAME, FixedRangeAllocationsFrom64To128)
{
    RangedTest(33, 128);
}
#endif

#if 0 FixedRangeAllocationsFrom128To256
TEST(TEST_SUITE_NAME, FixedRangeAllocationsFrom128To256)
{
    RangedTest(65, 256);
}
#endif

#if 0 FixedRangeAllocationsFrom256To512
TEST(TEST_SUITE_NAME, FixedRangeAllocationsFrom256To512)
{
    RangedTest(129, 512);
}
#endif

#if 0 FixedRangeAllocationsFrom512To1024
TEST(TEST_SUITE_NAME, FixedRangeAllocationsFrom512To1024)
{
    RangedTest(260, 1024);
}
#endif

#if 0 FixedRangeAllocationsFrom1024To2048
TEST(TEST_SUITE_NAME, FixedRangeAllocationsFrom1024To2048)
{
    RangedTest(513, 2048);
}
#endif

#if 0 FixedRangeAllocationsFrom2048To4096
TEST(TEST_SUITE_NAME, FixedRangeAllocationsFrom2048To4096)
{
    RangedTest(1025, 4096);
}
#endif

#if 0
 TEST(TEST_SUITE_NAME, AllocateStoresData)
{
     MM::MemoryManager manager(ManagerConfig);
     int* ptr = manager.allocate<int>(40);

     for (int i = 0; i < 40; i++)
     {
         ptr[i] = i;
     }

     for (int i = 0; i < 40; i++)
     {
         EXPECT_EQ(ptr[i], i) << "Data corrupted at index " << i;
     }
 }

 TEST(TEST_SUITE_NAME, MultipleAllocations)
{
     MM::MemoryManager manager(ManagerConfig);  // 1 KB

     int* ptr1 = manager.allocate<int>(10);
     float* ptr2 = manager.allocate<float>(20);

     EXPECT_NE(ptr1, nullptr);
     EXPECT_NE(ptr2, nullptr);

     EXPECT_LT(reinterpret_cast<uintptr_t>(ptr1), reinterpret_cast<uintptr_t>(ptr2));
 }

 TEST(TEST_SUITE_NAME, OverflowReturnsNull)
{
     MM::MemoryManager manager(ManagerConfig);

     int* ptr1 = manager.allocate<int>(10);
     EXPECT_NE(ptr1, nullptr);

     int* ptr2 = manager.allocate<int>(100);
     EXPECT_EQ(ptr2, nullptr);
 }

 TEST(TEST_SUITE_NAME, Alignment)
{
     MM::MemoryManager manager(ManagerConfig);

     double* ptr = manager.allocate<double>(1);

     EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % alignof(double), 0) << "Pointer is not properly aligned for double";
 }
#endif