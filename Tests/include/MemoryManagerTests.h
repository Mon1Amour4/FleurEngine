#pragma once

#include <random>

#include "MemoryManager.h"
#include "SyntheticTypes.hpp"
#include "gtest/gtest.h"

using SyntheticTypesVariant =
    std::variant<Synthetic::Synthetic1, Synthetic::Synthetic2, Synthetic::Synthetic3, Synthetic::Synthetic4, Synthetic::Synthetic5/*, Synthetic::Synthetic6,
                 Synthetic::Synthetic7, Synthetic::Synthetic8, Synthetic::Synthetic9, Synthetic::Synthetic10, Synthetic::Synthetic11*/>;

#define TEST_SUITE_NAME MemoryManagerTest
#define ManagerConfig 8 * 1024 * 1024, 16 * 1024, MM::PAGE_SIZE, MM::MIN_SLOT_SIZE

TEST(TEST_SUITE_NAME, AllocateNotNull)
{
    MM::MemoryManager manager(ManagerConfig);
    manager.ClearFile("MemorySnapshot.txt");
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> actionDist(0, 0);  // 0 = alloc, 1 = free
    std::uniform_int_distribution<> countDist(1, 10);
    std::uniform_int_distribution<> typeDist(0, 4);

    struct AllocRecord
    {
        int type;
        int count;
        void* ptr;
    };
    std::vector<AllocRecord> allocated;

    // helper: allocate N objects of the chosen synthetic type, return pointer as void*
    auto do_allocate = [&](int type, int count) -> void*
    {
        switch (type)
        {
        case 0:
            return static_cast<void*>(manager.allocate<Synthetic::Synthetic1>(count));
        case 1:
            return static_cast<void*>(manager.allocate<Synthetic::Synthetic2>(count));
        case 2:
            return static_cast<void*>(manager.allocate<Synthetic::Synthetic3>(count));
        case 3:
            return static_cast<void*>(manager.allocate<Synthetic::Synthetic4>(count));
        case 4:
            return static_cast<void*>(manager.allocate<Synthetic::Synthetic5>(count));

            return nullptr;
        }
    };

    // helper: deallocate using the exact type and count
    auto do_deallocate = [&](const AllocRecord& rec)
    {
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

    for (size_t i = 0; i < 100; i++)
    {
        bool doFree = (actionDist(gen) == 1);
        int count = countDist(gen);
        int type = typeDist(gen);

        if (doFree && !allocated.empty())
        {
            std::uniform_int_distribution<size_t> freeIndexDist(0, allocated.size() - 1);
            size_t idx = freeIndexDist(gen);
            AllocRecord rec = allocated[idx];
            do_deallocate(rec);

            if (idx + 1 != allocated.size())
                std::swap(allocated[idx], allocated.back());
            allocated.pop_back();
        }
        else
        {
            // MyFile << "\nAllocation{type: " << std::to_string(type) << ", count: " << std::to_string(count) << "}\n";
            std::cout << "\nAllocation{type: " << std::to_string(type) << ", count: " << std::to_string(count) << "}\n";
            void* ptr = do_allocate(type, count);
            ASSERT_NE(ptr, nullptr);
            allocated.push_back(AllocRecord{type, count, ptr});
        }
        std::cout << i;
        manager.Print();
        manager.SaveSnapshotToFile("MemorySnapshot.txt");
    }
    for (const auto& rec : allocated) do_deallocate(rec);
    allocated.clear();
}

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