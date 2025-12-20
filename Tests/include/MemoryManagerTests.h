#pragma once

#include <random>

#include "MemoryManager.h"
#include "SyntheticTypes.hpp"
#include "gtest/gtest.h"

#define TEST_SUITE_NAME MemoryManagerTest
#define ManagerConfig 1024ULL * 1024ULL * 1024ULL * 5ULL

void RangedTest(uint32_t from, uint32_t to)
{
    auto manager = MM::MemoryManager::ManagerFabric(ManagerConfig);
    MemoryInfo* memoryInfo = new MemoryInfo();

    // Clear file:
    /*std::ofstream myFile;
    myFile.open("MemorySnapshot.txt");
    myFile.close();*/

    size_t allocated = 0;
    size_t allocationsCupBytes = 1024ul * 1024ul * 500ul;

    MM::Benchmark mark{2};
    struct info
    {
        int* ptr;
        int count;
        size_t size;
    };
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> actionDist(from, to);
    std::vector<info> mm_pairs;
    std::vector<info> std_pairs;

    std::cout << "Memory Manager benchmark\n";
    size_t counter = 0;
    for (size_t i = 0; allocated < allocationsCupBytes; i++)
    {
        size_t size = (actionDist(gen) / sizeof(int) * sizeof(int));
        int count = size / sizeof(int);

        if (size > 2032)
        {
            uint32_t power = 0;
            Fleur::Core::bit_scan_reverse(size + sizeof(TLSFAllocator::free_block_header), &power);
            size = pow(2, power);
        }
        else
        {
            size = MM::AlignTo(sizeof(int) * count, 8);
        }
        allocated += size;
        mark.StartAlloc();
        mm_pairs.push_back({manager->allocate<int>(count), count, size});
        mark.EndAlloc();

        memoryInfo->AddAlloc(size);
    }

    for (auto alloc : mm_pairs)
    {
        mark.StartDealloc();
        manager->deallocate<int>(alloc.ptr, alloc.count);
        mark.EndDealloc();
        memoryInfo->AddDealloc(alloc.size);

        counter++;

        /*std::ofstream myFile;
        myFile.open("MemorySnapshot.txt", std::ios_base::app);
        myFile << manager->GetSnapshot();
        myFile.close();*/
    }
    mark.Print();

    std::allocator<int> alloc;

    std::cout << "STD allocator benchmark\n";
    MM::Benchmark std_mark{2};
    for (size_t i = 0; i < mm_pairs.size(); i++)
    {
        std_mark.StartAlloc();
        int count = mm_pairs[i].count;
        std_pairs.push_back({alloc.allocate(count), count, mm_pairs[i].size});
        std_mark.EndAlloc();
    }
    for (size_t i = 0; i < mm_pairs.size(); i++)
    {
        std_mark.StartDealloc();
        alloc.deallocate(std_pairs[i].ptr, std_pairs[i].count);
        std_mark.EndDealloc();
    }
    std_mark.Print();

    memoryInfo->Print();

    manager->~MemoryManager();
}

#if 1 MemoryManagerRandomAllocationTestBenchmark
TEST(TEST_SUITE_NAME, MemoryManagerRandomAllocationTestBenchmark)
{
    using namespace MM;

    MM::MemoryManager* manager = MM::MemoryManager::ManagerFabric(ManagerConfig);

    // Clear file :
    // std::ofstream myFile;
    // myFile.open("MemorySnapshot.txt");
    // myFile.close();

    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> actionDist(0, 1);
    std::uniform_int_distribution<> sizeDist(4, 4'194'303);

    struct Info
    {
        bool alloc;
        std::pair<void*, uint32_t> pair;
        uint32_t id;
    };

    std::vector<Info> pairs;
    std::vector<Info> std_pairs;

    size_t allocationCap = 1024ul * 1024ul * 1024ul;

    Benchmark mm_mark{2};
    static uint32_t id = 0;
    for (size_t currentCap = 0; currentCap < allocationCap;)
    {
        bool doFree = (actionDist(gen) == 0);
        int size = sizeDist(gen);

        if (!doFree)
        {
            // Allocation
            uint32_t count = size / sizeof(int);
            int reminder = size % sizeof(int);
            if (reminder > 0)
                count++;

            mm_mark.StartAlloc();
            pairs.emplace_back(true, std::make_pair((void*)manager->allocate<int>(count), count), id);
            mm_mark.EndAlloc();
            id++;
            currentCap += count * sizeof(int);

            std_pairs.emplace_back(pairs.back());
        }
        else
        {
            if (!pairs.empty())
            {
                auto info = pairs.back();
                pairs.pop_back();

                mm_mark.StartDealloc();
                manager->deallocate<int>(info.pair.first, info.pair.second);
                mm_mark.EndDealloc();

                Info deallocInfo = info;
                deallocInfo.alloc = false;
                std_pairs.emplace_back(deallocInfo);
            }
        }
        // std::ofstream myFile;
        // myFile.open("MemorySnapshot.txt", std::ios_base::app);
        // myFile << manager->GetSnapshot();
        // myFile.close();
    }
    // std::allocator
    std::allocator<int> alloc;
    std::unordered_map<uint32_t, void*> helper;

    Benchmark std_mark{2};
    for (const auto& info : std_pairs)
    {
        if (info.alloc)
        {
            std_mark.StartAlloc();
            helper.emplace(info.id, alloc.allocate(info.pair.second));
            std_mark.EndAlloc();
        }
        else
        {
            auto it = helper.find(info.id);
            if (it == helper.end())
                continue;

            std_mark.StartDealloc();
            alloc.deallocate((int*)it->second, info.pair.second);
            std_mark.EndDealloc();
            helper.erase(it);
        }
    }

    mm_mark.Print();
    std_mark.Print();
}
#endif

#if 0 TLSFManualTests
TEST(TEST_SUITE_NAME, TLSFManualTests)
{
    using namespace MM;

    MM::MemoryManager* manager = MM::MemoryManager::ManagerFabric(ManagerConfig);

    // Clear file:
    std::ofstream myFile;
    myFile.open("MemorySnapshot.txt");
    myFile.close();

    auto deallocate = [manager](void* ptr, uint32_t count) -> void
    {
        manager->deallocate<int>(ptr, count);
        std::ofstream myFile;
        myFile.open("MemorySnapshot.txt", std::ios_base::app);
        myFile << manager->GetSnapshot();
        myFile.close();
    };
    auto allocate = [manager](uint32_t count) -> void*
    {
        void* ptr = manager->allocate<int>(count);
        std::ofstream myFile;
        myFile.open("MemorySnapshot.txt", std::ios_base::app);
        myFile << manager->GetSnapshot();
        myFile.close();
        return ptr;
    };
    auto alloc_dealloc = [manager](uint32_t count)
    {
        void* ptr = manager->allocate<int>(count);
        {
            std::ofstream myFile;
            myFile.open("MemorySnapshot.txt", std::ios_base::app);
            myFile << manager->GetSnapshot();
            myFile.close();
        }

        manager->deallocate<int>(ptr, count);

        {
            std::ofstream myFile;
            myFile.open("MemorySnapshot.txt", std::ios_base::app);
            myFile << manager->GetSnapshot();
            myFile.close();
        }
    };
    auto tripple_alloc_dealloc = [manager](uint32_t count, const char* message)
    {
        void* ptr_1 = manager->allocate<int>(count);
        {
            std::ofstream myFile;
            myFile.open("MemorySnapshot.txt", std::ios_base::app);
            myFile << message;
            myFile << manager->GetSnapshot();
            myFile.close();
        }

        void* ptr_2 = manager->allocate<int>(count);

        {
            std::ofstream myFile;
            myFile.open("MemorySnapshot.txt", std::ios_base::app);
            myFile << manager->GetSnapshot();
            myFile.close();
        }

        void* ptr_3 = manager->allocate<int>(count);

        {
            std::ofstream myFile;
            myFile.open("MemorySnapshot.txt", std::ios_base::app);
            myFile << manager->GetSnapshot();
            myFile.close();
        }

        manager->deallocate<int>(ptr_1, count);

        {
            std::ofstream myFile;
            myFile.open("MemorySnapshot.txt", std::ios_base::app);
            myFile << manager->GetSnapshot();
            myFile.close();
        }

        manager->deallocate<int>(ptr_2, count);

        {
            std::ofstream myFile;
            myFile.open("MemorySnapshot.txt", std::ios_base::app);
            myFile << manager->GetSnapshot();
            myFile.close();
        }

        manager->deallocate<int>(ptr_3, count);

        {
            std::ofstream myFile;
            myFile.open("MemorySnapshot.txt", std::ios_base::app);
            myFile << manager->GetSnapshot();
            myFile.close();
        }
    };

    // 2^11 + 0*2^6 -> [11][1]
    alloc_dealloc(512);

    // 2^11 + 0*2^6
    tripple_alloc_dealloc(512, "512\n\0");

    // 2^11 + 1*2^6
    tripple_alloc_dealloc(528, "528\n\0");

    // 2^11 + 15*2^6
    tripple_alloc_dealloc(752, "752\n\0");

    // 2^11 + 31*2^6
    tripple_alloc_dealloc(1000, "1008\n\0");

    // (2^11 + 31*2^6) + 4
    tripple_alloc_dealloc(1009, "1009\n\0");

    // test search_suitable_block
    tripple_alloc_dealloc(736, "test search_suitable_block\n\0");
    tripple_alloc_dealloc(752, "752\n\0");
    tripple_alloc_dealloc(728, "728\n\0");

    // test split
    // 2^21 + 15*2^16 -> [21][16]
    alloc_dealloc(770048);

    // 2^21 + 0*2^16 -> [21][15], remaining[19][28]
    auto ptr_1 = allocate(524288);

    // 2^19 + 27*2^14 ->[14][0], remaining [13][31]
    auto _ptr2 = allocate(241664);


    deallocate(_ptr2, 241664);

    // [21][0]
    deallocate(ptr_1, 524288);
}
#endif

#if 0 TLSFRandomTests
TEST(TEST_SUITE_NAME, TLSFRandomTests)
{
    using namespace MM;

    MM::MemoryManager* manager = MM::MemoryManager::ManagerFabric(ManagerConfig);

    // Clear file :
    // std::ofstream myFile;
    // myFile.open("MemorySnapshot.txt");
    // myFile.close();

    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> actionDist(0, 1);
    std::uniform_int_distribution<> sizeDist(2033, 4'194'303);

    std::vector<std::pair<void*, uint32_t>> pairs;
    Benchmark mark{1};
    size_t allocationCap = 1024ul * 1024ul * 1024ul * 3ul;
    for (size_t currentCap = 0; currentCap < allocationCap;)
    {
        bool doFree = (actionDist(gen) == 0);
        int size = sizeDist(gen);

        if (!doFree)
        {
            // Allocation
            uint32_t count = size / sizeof(int);
            int reminder = size % sizeof(int);
            if (reminder > 0)
                count++;

            mark.StartAlloc();
            pairs.emplace_back(std::make_pair((void*)manager->allocate<int>(count), count));
            mark.EndAlloc();

            currentCap += count * sizeof(int);
        }
        else
        {
            if (!pairs.empty())
            {
                auto pair = pairs.back();
                mark.StartAlloc();
                manager->deallocate<int>(pair.first, pair.second);
                mark.EndDealloc();

                pairs.pop_back();
            }
        }

        // std::ofstream myFile;
        // myFile.open("MemorySnapshot.txt", std::ios_base::app);
        // myFile << manager->GetSnapshot();
        // myFile.close();
    }
    mark.Print();
}
#endif

#if 0 ChunksFreeListTest
TEST(TEST_SUITE_NAME, ChunksFreeListTest)
{
    using namespace MM;
    MemoryManager manager(ManagerConfig);
    manager.ClearFile("MemorySnapshot.txt");

    // First Chunk
    std::vector<int*> ptrs;
    for (size_t i = 0; i < 16; i++)
    {
        ptrs.push_back(manager.allocate<int>(64));
    }
    manager.SaveSnapshotToFile("MemorySnapshot.txt", 1);

    // Second Chunk
    std::vector<int*> ptrs2;
    for (size_t i = 0; i < 16; i++)
    {
        ptrs2.push_back(manager.allocate<int>(64));
    }
    manager.SaveSnapshotToFile("MemorySnapshot.txt", 2);

    // Third Chunk
    std::vector<int*> ptrs3;
    for (size_t i = 0; i < 16; i++)
    {
        ptrs3.push_back(manager.allocate<int>(64));
    }
    manager.SaveSnapshotToFile("MemorySnapshot.txt", 3);

    manager.deallocate<int>(static_cast<void*>(ptrs2.back()), 64);
    ptrs2.pop_back();
    manager.SaveSnapshotToFile("MemorySnapshot.txt", 4);

    manager.deallocate<int>(static_cast<void*>(ptrs.back()), 64);
    ptrs.pop_back();
    manager.SaveSnapshotToFile("MemorySnapshot.txt", 5);

    manager.deallocate<int>(static_cast<void*>(ptrs3.back()), 64);
    ptrs3.pop_back();
    manager.SaveSnapshotToFile("MemorySnapshot.txt", 6);

    ptrs3.push_back(manager.allocate<int>(64));
    manager.SaveSnapshotToFile("MemorySnapshot.txt", 7);

    ptrs2.push_back(manager.allocate<int>(64));
    manager.SaveSnapshotToFile("MemorySnapshot.txt", 8);

    ptrs2.push_back(manager.allocate<int>(64));
    manager.SaveSnapshotToFile("MemorySnapshot.txt", 9);

    ptrs2.push_back(manager.allocate<int>(64));
    manager.SaveSnapshotToFile("MemorySnapshot.txt", 10);
}
#endif

#if 0 Array Allocations
TEST(TEST_SUITE_NAME, ArrayAllocations)
{
    using namespace MM;
    MemoryManager manager(ManagerConfig);

    // Clear file:
    std::ofstream myFile;
    myFile.open("MemorySnapshot.txt");
    myFile.close();

    auto ptr = manager.allocate<Synthetic::Synthetic1>(10);
    manager.deallocate<Synthetic::Synthetic1>(ptr, 10);
}
#endif

#if 0 Random Allocations
TEST(TEST_SUITE_NAME, RandomAllocations)
{
    using namespace MM;
    std::ofstream myFile;
    myFile.open("MemorySnapshot.txt");
    myFile.close();
    MM::MemoryManager* manager = MM::MemoryManager::ManagerFabric(ManagerConfig);

    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> actionDist(0, 1);
    std::uniform_int_distribution<uint32_t> sizeDist(4, 4'194'304);

    struct Op
    {
        bool alloc;
        uint32_t count;
        uint32_t id;
    };

    struct Live
    {
        void* ptr;
        uint32_t count;
        uint32_t id;
    };

    std::vector<Op> ops;
    std::vector<Live> live;

    ops.reserve(1'000'000);
    live.reserve(1'000'000);

    uint32_t idCounter = 0;
    size_t liveBytes = 0;

    constexpr size_t allocCap = 1024ull * 1024ull * 1024ull;  // 1 GB

    Benchmark mm_mark{2};

    /* =========================
       Phase 1: record scenario
       ========================= */
    while (liveBytes < allocCap)
    {
        bool doFree = (actionDist(gen) == 0);

        if (doFree && !live.empty())
        {
            std::uniform_int_distribution<size_t> freeIdx(0, live.size() - 1);
            size_t idx = freeIdx(gen);

            auto rec = live[idx];

            mm_mark.StartDealloc();
            manager->deallocate<int>(rec.ptr, rec.count);
            mm_mark.EndDealloc();

            ops.push_back({false, rec.count, rec.id});
            liveBytes -= rec.count * sizeof(int);

            live[idx] = live.back();
            live.pop_back();
        }
        else
        {
            uint32_t size = sizeDist(gen);
            uint32_t count = (size / sizeof(int));
            if (count == 0)
                continue;

            mm_mark.StartAlloc();
            void* ptr = manager->allocate<int>(count);
            mm_mark.EndAlloc();

            if (!ptr)
                continue;

            ++idCounter;

            ops.push_back({true, count, idCounter});
            live.push_back({ptr, count, idCounter});
            liveBytes += count * sizeof(int);
        }
        std::ofstream myFile;
        myFile.open("MemorySnapshot.txt", std::ios_base::app);
        myFile << manager->GetSnapshot();
        myFile.close();
    }

    mm_mark.Print();

    /* =========================
       Phase 2: replay for std
       ========================= */
    Benchmark std_mark{2};
    std::allocator<int> stdAlloc;
    std::unordered_map<uint32_t, int*> stdLive;
    stdLive.reserve(ops.size());

    for (const auto& op : ops)
    {
        if (op.alloc)
        {
            std_mark.StartAlloc();
            int* ptr = stdAlloc.allocate(op.count);
            std_mark.EndAlloc();

            stdLive[op.id] = ptr;
        }
        else
        {
            auto it = stdLive.find(op.id);
            if (it != stdLive.end())
            {
                std_mark.StartDealloc();
                stdAlloc.deallocate(it->second, op.count);
                std_mark.EndDealloc();

                stdLive.erase(it);
            }
        }
    }

    std_mark.Print();
}
#endif

#if 0 FixedRangeAllocationsFrom2048To2097152
TEST(TEST_SUITE_NAME, FixedRangeAllocationsFrom2048To2097152)
{
    RangedTest(2048, 2097152);
}
#endif

#if 0 FixedRangeAllocationsFrom64To86
TEST(TEST_SUITE_NAME, FixedRangeAllocationsFrom64To128)
{
    RangedTest(33, 46);
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