#pragma once

#include "MemoryManager.h"
#include "gtest/gtest.h"

#define TEST_SUITE_NAME MemoryManagerTest

TEST(TEST_SUITE_NAME, AllocateNotNull)
{
    MM::MemoryManager<8 * 1024 * 1024> manager;
    int* ptr = manager.allocate<int, 40>();
    manager.Print();

    manager.deallocate<int, 40>(ptr);
    manager.Print();

    manager.allocate<int, 100>();
    manager.Print();

    EXPECT_NE(ptr, nullptr) << "Allocate returned nullptr for valid request";
    manager.Print();
}

TEST(TEST_SUITE_NAME, AllocateStoresData)
{
    MM::MemoryManager<8 * 1024 * 1024> manager;
    int* ptr = manager.allocate<int, 40>();

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
    MM::MemoryManager<1024> manager;  // 1 KB

    int* ptr1 = manager.allocate<int, 10>();
    float* ptr2 = manager.allocate<float, 20>();

    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);

    EXPECT_LT(reinterpret_cast<uintptr_t>(ptr1), reinterpret_cast<uintptr_t>(ptr2));
}

TEST(TEST_SUITE_NAME, OverflowReturnsNull)
{
    MM::MemoryManager<64> manager;

    int* ptr1 = manager.allocate<int, 10>();
    EXPECT_NE(ptr1, nullptr);

    int* ptr2 = manager.allocate<int, 100>();
    EXPECT_EQ(ptr2, nullptr);
}

TEST(TEST_SUITE_NAME, Alignment)
{
    MM::MemoryManager<1024> manager;

    double* ptr = manager.allocate<double, 1>();

    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % alignof(double), 0) << "Pointer is not properly aligned for double";
}