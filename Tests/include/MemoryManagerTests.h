#pragma once

#include <random>

#include "MemoryManager.h"
#include "SyntheticTypes.hpp"
#include "gtest/gtest.h"

#define TEST_SUITE_NAME MemoryManagerTest
#define ManagerConfig 1024ULL * 1024ULL * 1024ULL * 5ULL, 1024ULL * 1024ULL * 1024ULL * 2ULL, MM::PAGE_SIZE, MM::MIN_SLOT_SIZE

#if 0
TEST(TEST_SUITE_NAME, RandomAllocations)
{
    MM::MemoryManager manager(ManagerConfig);
    manager.ClearFile("MemorySnapshot.txt");
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> actionDist(0, 1);  // 0 = alloc, 1 = free
    std::uniform_int_distribution<> countDist(1, 10);
    std::uniform_int_distribution<> typeDist(0, 4);

    struct AllocRecord
    {
        int type;
        int count;
        uint32_t id;
        void* ptr;
    };
    std::vector<AllocRecord> allocated;
    std::vector<AllocRecord> allocated2;
    // free - false
    // alloc - true
    std::vector<std::pair<uint32_t, bool>> alloc_free;
    static uint32_t idCounter = 0;
    // helper: allocate N objects of the chosen synthetic type, return pointer as void*
    auto do_allocate = [&](int type, int count, uint32_t id) -> void*
    {
        // MM_PRINT("\nAllocation{type: " << std::to_string(type) << ", count: " << std::to_string(count) << ", id{" << std::to_string(id) << "}\n")
        switch (type)
        {
        case 0:
        {
            // MM_PRINT("Allocation size: " << sizeof(Synthetic::Synthetic1) * count << "\n")
            return static_cast<void*>(manager.allocate<Synthetic::Synthetic1>(count));
        }
        case 1:
        {
            // MM_PRINT("Allocation size: " << sizeof(Synthetic::Synthetic2) * count << "\n")
            return static_cast<void*>(manager.allocate<Synthetic::Synthetic2>(count));
        }
        case 2:
        {
            // MM_PRINT("Allocation size: " << sizeof(Synthetic::Synthetic3) * count << "\n")
            return static_cast<void*>(manager.allocate<Synthetic::Synthetic3>(count));
        }
        case 3:
        {
            // MM_PRINT("Allocation size: " << sizeof(Synthetic::Synthetic4) * count << "\n")
            return static_cast<void*>(manager.allocate<Synthetic::Synthetic4>(count));
        }
        case 4:
        {
            // MM_PRINT("Allocation size: " << sizeof(Synthetic::Synthetic5) * count << "\n")
            return static_cast<void*>(manager.allocate<Synthetic::Synthetic5>(count));
        }

            return nullptr;
        }
    };
    auto do_allocate_std = [&](int type, int count, uint32_t id) -> void*
    {
        // MM_PRINT("\nAllocation{type: " << std::to_string(type) << ", count: " << std::to_string(count) << ", id{" << std::to_string(id) << "}\n")
        switch (type)
        {
        case 0:
        {
            // MM_PRINT("Allocation size: " << sizeof(Synthetic::Synthetic1) * count << "\n")
            std::allocator<Synthetic::Synthetic1> alloc;
            return static_cast<void*>(alloc.allocate(count));
        }
        case 1:
        {
            // MM_PRINT("Allocation size: " << sizeof(Synthetic::Synthetic2) * count << "\n")
            std::allocator<Synthetic::Synthetic2> alloc;
            return static_cast<void*>(alloc.allocate(count));
        }
        case 2:
        {
            // MM_PRINT("Allocation size: " << sizeof(Synthetic::Synthetic3) * count << "\n")
            std::allocator<Synthetic::Synthetic3> alloc;
            return static_cast<void*>(alloc.allocate(count));
        }
        case 3:
        {
            // MM_PRINT("Allocation size: " << sizeof(Synthetic::Synthetic4) * count << "\n")
            std::allocator<Synthetic::Synthetic4> alloc;
            return static_cast<void*>(alloc.allocate(count));
        }
        case 4:
        {
            // MM_PRINT("Allocation size: " << sizeof(Synthetic::Synthetic5) * count << "\n")
            std::allocator<Synthetic::Synthetic5> alloc;
            return static_cast<void*>(alloc.allocate(count));
        }

            return nullptr;
        }
    };

    // helper: deallocate using the exact type and count
    auto do_deallocate = [&](const AllocRecord& rec)
    {
        // MM_PRINT("\nDeallocation{type: " << std::to_string(rec.type) << ", count: " << std::to_string(rec.count) << ", id{" << std::to_string(rec.id) << "}\n
        // ")

        switch (rec.type)
        {
        case 0:
            manager.deallocate<Synthetic::Synthetic1>(static_cast<Synthetic::Synthetic1*>(rec.ptr), rec.count);
            break;
        case 1:
            manager.deallocate<Synthetic::Synthetic2>(static_cast<Synthetic::Synthetic2*>(rec.ptr), rec.count);
            break;
        case 2:
            manager.deallocate<Synthetic::Synthetic3>(static_cast<Synthetic::Synthetic3*>(rec.ptr), rec.count);
            break;
        case 3:
            manager.deallocate<Synthetic::Synthetic4>(static_cast<Synthetic::Synthetic4*>(rec.ptr), rec.count);
            break;
        case 4:
            manager.deallocate<Synthetic::Synthetic5>(static_cast<Synthetic::Synthetic5*>(rec.ptr), rec.count);
            break;

        default: /* unreachable */
            break;
        }
    };
    auto do_deallocate_std = [&](const AllocRecord& rec)
    {
        // MM_PRINT("\nDeallocation{type: " << std::to_string(rec.type) << ", count: " << std::to_string(rec.count) << ", id{" << std::to_string(rec.id) << "}\n
        // ")

        switch (rec.type)
        {
        case 0:
        {
            std::allocator<Synthetic::Synthetic1> alloc;
            alloc.deallocate(reinterpret_cast<Synthetic::Synthetic1*>(rec.ptr), rec.count);
            break;
        }
        case 1:
        {
            std::allocator<Synthetic::Synthetic2> alloc;
            alloc.deallocate(reinterpret_cast<Synthetic::Synthetic2*>(rec.ptr), rec.count);
            break;
        }
        case 2:
        {
            std::allocator<Synthetic::Synthetic3> alloc;
            alloc.deallocate(reinterpret_cast<Synthetic::Synthetic3*>(rec.ptr), rec.count);
            break;
        }
        case 3:
        {
            std::allocator<Synthetic::Synthetic4> alloc;
            alloc.deallocate(reinterpret_cast<Synthetic::Synthetic4*>(rec.ptr), rec.count);
            break;
        }
        case 4:
        {
            std::allocator<Synthetic::Synthetic5> alloc;
            alloc.deallocate(reinterpret_cast<Synthetic::Synthetic5*>(rec.ptr), rec.count);
            break;
        }

        default: /* unreachable */
            break;
        }
    };
    MM::Benchmark mark{2};
    for (size_t i = 0; i < 1000; i++)
    {
        auto start = std::chrono::steady_clock::now();

        bool doFree = (actionDist(gen) == 1);
        int count = countDist(gen);
        int type = typeDist(gen);

        if (doFree && !allocated.empty())
        {
            std::uniform_int_distribution<size_t> freeIndexDist(0, allocated.size() - 1);
            size_t idx = freeIndexDist(gen);
            AllocRecord rec = allocated[idx];

            mark.StartDealloc();
            do_deallocate(rec);
            mark.EndDealloc();

            alloc_free.push_back({rec.id, false});

            if (idx + 1 != allocated.size())
                std::swap(allocated[idx], allocated.back());
            allocated.pop_back();
        }
        else
        {
            idCounter++;

            mark.StartAlloc();
            void* ptr = do_allocate(type, count, idCounter);
            mark.EndAlloc();

            // ASSERT_NE(ptr, nullptr);
            allocated.push_back(AllocRecord{type, count, idCounter, ptr});
            allocated2.push_back(AllocRecord{type, count, idCounter, ptr});
            alloc_free.push_back({idCounter, true});
        }

        auto end = std::chrono::steady_clock::now();
        float seconds = std::chrono::duration<float>(end - start).count();
        mark.Tick(seconds);

        MM_PRINT(std::to_string(i) << "\n")
        // manager.Print();
        // manager.SaveSnapshotToFile("MemorySnapshot.txt", i);
    }
    MM_PRINT("---------------- PURGE --------------\n");
    mark.Print();
    allocated.clear();

    MM::Benchmark std_mark(2);
    for (auto pair : alloc_free)
    {
        if (pair.second)
        {
            // Alloc

            auto it = std::find_if(allocated2.begin(), allocated2.end(), [&](const AllocRecord& r) { return r.id == pair.first; });
            if (it == allocated2.end())
                continue;

            auto start = std::chrono::steady_clock::now();
            std_mark.StartAlloc();
            void* ptr = do_allocate_std(it->type, it->count, it->id);
            std_mark.EndAlloc();


            auto end = std::chrono::steady_clock::now();
            float seconds = std::chrono::duration<float>(end - start).count();
            std_mark.Tick(seconds);

            it->ptr = ptr;
        }
        else
        {
            // Dealloc
            auto it = std::find_if(allocated2.begin(), allocated2.end(), [&](const AllocRecord& r) { return r.id == pair.first; });
            if (it == allocated2.end())
                continue;

            auto start = std::chrono::steady_clock::now();

            std_mark.StartDealloc();
            do_deallocate_std(*it);
            std_mark.EndDealloc();

            auto end = std::chrono::steady_clock::now();
            float seconds = std::chrono::duration<float>(end - start).count();
            mark.Tick(seconds);
        }
    }
    std_mark.Print();
}
#endif

#if 1
TEST(TEST_SUITE_NAME, FixedRangeAllocationsFrom64To128)
{
    MM::MemoryManager manager(ManagerConfig);
    size_t allocated = 0;
    uint32_t from = 64;
    uint32_t to = 128;
    uint32_t allocationsCupBytes = 1024 * 1024 * 1024;

    MM::Benchmark mark{2};

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> actionDist(0, to - from);
    std::vector<std::pair<int, int*>> mm_pairs;
    std::vector<std::pair<int, int*>> std_pairs;
    std::cout << "Memory Manager benchmark\n";
    for (size_t i = 0; allocated < allocationsCupBytes; i++)
    {
        int count = from + actionDist(gen);
        allocated += count;

        mark.StartAlloc();
        mm_pairs.push_back({count, manager.allocate<int>(count / sizeof(int))});
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
    for (size_t i = 0; i < mm_pairs.size() - 1; i++)
    {
        std_mark.StartAlloc();
        uint32_t count = mm_pairs[i].first;
        std_pairs.push_back({count, alloc.allocate(count)});
        std_mark.EndAlloc();
    }
    for (size_t i = 0; i < mm_pairs.size() - 1; i++)
    {
        std_mark.StartDealloc();
        alloc.deallocate(std_pairs[i].second, std_pairs[i].first);
        std_mark.EndDealloc();
    }
    std_mark.Print();
}
#endif

#if 0
TEST(TEST_SUITE_NAME, FixedRangeAllocationsFrom128To256)
{
    MM::MemoryManager manager(ManagerConfig);
    size_t allocated = 0;
    uint32_t from = 128;
    uint32_t to = 256;
    uint32_t allocationsCupBytes = 1024 * 1024 * 1024;

    MM::Benchmark mark{2};

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> actionDist(0, to - from);
    std::vector<std::pair<int, int*>> mm_pairs;
    std::vector<std::pair<int, int*>> std_pairs;
    std::cout << "Memory Manager benchmark\n";
    for (size_t i = 0; allocated < allocationsCupBytes; i++)
    {
        int count = from + actionDist(gen);
        allocated += count;

        mark.StartAlloc();
        mm_pairs.push_back({count, manager.allocate<int>(count / sizeof(int))});
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
    for (size_t i = 0; i < mm_pairs.size() - 1; i++)
    {
        std_mark.StartAlloc();
        uint32_t count = mm_pairs[i].first;
        std_pairs.push_back({count, alloc.allocate(count)});
        std_mark.EndAlloc();
    }
    for (size_t i = 0; i < mm_pairs.size() - 1; i++)
    {
        std_mark.StartDealloc();
        alloc.deallocate(std_pairs[i].second, std_pairs[i].first);
        std_mark.EndDealloc();
    }
    std_mark.Print();
}
#endif

#if 0
TEST(TEST_SUITE_NAME, FixedRangeAllocationsFrom256To512)
{
    MM::MemoryManager manager(ManagerConfig);
    size_t allocated = 0;
    uint32_t from = 256;
    uint32_t to = 512;
    uint32_t allocationsCupBytes = 1024 * 1024 * 1024;

    MM::Benchmark mark{2};

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> actionDist(0, to - from);
    std::vector<std::pair<int, int*>> mm_pairs;
    std::vector<std::pair<int, int*>> std_pairs;
    std::cout << "Memory Manager benchmark\n";
    for (size_t i = 0; allocated < allocationsCupBytes; i++)
    {
        int count = from + actionDist(gen);
        allocated += count;

        mark.StartAlloc();
        mm_pairs.push_back({count, manager.allocate<int>(count / sizeof(int))});
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
    for (size_t i = 0; i < mm_pairs.size() - 1; i++)
    {
        std_mark.StartAlloc();
        uint32_t count = mm_pairs[i].first;
        std_pairs.push_back({count, alloc.allocate(count)});
        std_mark.EndAlloc();
    }
    for (size_t i = 0; i < mm_pairs.size() - 1; i++)
    {
        std_mark.StartDealloc();
        alloc.deallocate(std_pairs[i].second, std_pairs[i].first);
        std_mark.EndDealloc();
    }
    std_mark.Print();
}
#endif

#if 0
TEST(TEST_SUITE_NAME, FixedRangeAllocationsFrom512To1024)
{
    MM::MemoryManager manager(ManagerConfig);
    size_t allocated = 0;
    uint32_t from = 256;
    uint32_t to = 512;
    uint32_t allocationsCupBytes = 1024 * 1024 * 1024;

    MM::Benchmark mark{2};

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> actionDist(0, to - from);
    std::vector<std::pair<int, int*>> mm_pairs;
    std::vector<std::pair<int, int*>> std_pairs;
    std::cout << "Memory Manager benchmark\n";
    for (size_t i = 0; allocated < allocationsCupBytes; i++)
    {
        int count = from + actionDist(gen);
        allocated += count;

        mark.StartAlloc();
        mm_pairs.push_back({count, manager.allocate<int>(count / sizeof(int))});
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
    for (size_t i = 0; i < mm_pairs.size() - 1; i++)
    {
        std_mark.StartAlloc();
        uint32_t count = mm_pairs[i].first;
        std_pairs.push_back({count, alloc.allocate(count)});
        std_mark.EndAlloc();
    }
    for (size_t i = 0; i < mm_pairs.size() - 1; i++)
    {
        std_mark.StartDealloc();
        alloc.deallocate(std_pairs[i].second, std_pairs[i].first);
        std_mark.EndDealloc();
    }
    std_mark.Print();
}
#endif

#if 0
TEST(TEST_SUITE_NAME, FixedRangeAllocationsFrom1024To2048)
{
    MM::MemoryManager manager(ManagerConfig);
    size_t allocated = 0;
    uint32_t from = 1024;
    uint32_t to = 2048;
    uint32_t allocationsCupBytes = 1024 * 1024 * 1024;

    MM::Benchmark mark{2};

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> actionDist(0, to - from);
    std::vector<std::pair<int, int*>> mm_pairs;
    std::vector<std::pair<int, int*>> std_pairs;
    std::cout << "Memory Manager benchmark\n";
    for (size_t i = 0; allocated < allocationsCupBytes; i++)
    {
        int count = from + actionDist(gen);
        allocated += count;

        mark.StartAlloc();
        mm_pairs.push_back({count, manager.allocate<int>(count / sizeof(int))});
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
    for (size_t i = 0; i < mm_pairs.size() - 1; i++)
    {
        std_mark.StartAlloc();
        uint32_t count = mm_pairs[i].first;
        std_pairs.push_back({count, alloc.allocate(count)});
        std_mark.EndAlloc();
    }
    for (size_t i = 0; i < mm_pairs.size() - 1; i++)
    {
        std_mark.StartDealloc();
        alloc.deallocate(std_pairs[i].second, std_pairs[i].first);
        std_mark.EndDealloc();
    }
    std_mark.Print();
}
#endif

#if 0
TEST(TEST_SUITE_NAME, FixedRangeAllocationsFrom2048To4096)
{
    MM::MemoryManager manager(ManagerConfig);
    size_t allocated = 0;
    uint32_t from = 2048;
    uint32_t to = 4096;
    uint32_t allocationsCupBytes = 1024 * 1024 * 1024;

    MM::Benchmark mark{2};

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> actionDist(0, to - from);
    std::vector<std::pair<int, int*>> mm_pairs;
    std::vector<std::pair<int, int*>> std_pairs;
    std::cout << "Memory Manager benchmark\n";
    for (size_t i = 0; allocated < allocationsCupBytes; i++)
    {
        int count = from + actionDist(gen);
        allocated += count;

        mark.StartAlloc();
        mm_pairs.push_back({count, manager.allocate<int>(count / sizeof(int))});
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
    for (size_t i = 0; i < mm_pairs.size() - 1; i++)
    {
        std_mark.StartAlloc();
        uint32_t count = mm_pairs[i].first;
        std_pairs.push_back({count, alloc.allocate(count)});
        std_mark.EndAlloc();
    }
    for (size_t i = 0; i < mm_pairs.size() - 1; i++)
    {
        std_mark.StartDealloc();
        alloc.deallocate(std_pairs[i].second, std_pairs[i].first);
        std_mark.EndDealloc();
    }
    std_mark.Print();
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