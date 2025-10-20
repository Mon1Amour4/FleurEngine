#pragma once

#include <fstream>
#include <iostream>
#include <limits>
#include <new>

#include "../Engine/Fleur/Concepts.hpp"
#include "BitSet64.h"

#pragma region MemoryManager Debug profiling definitions
//======================================================================
#if _DEBUG && 1 /*MEMORYMANAGER_PROFILING*/
#define MM_ASSERT(expression) \
    do                        \
    {                         \
        assert(expression);   \
    } while (0);

#define MM_DEBUG_BREAK(expression) \
    do                             \
    {                              \
        if (expression == true)    \
        {                          \
            __debugbreak();        \
        }                          \
    } while (0);

#define MM_DEBUG_EXPRESSION(code) \
    do                            \
    {                             \
        code;                     \
    } while (0);

#define MM_PRINT(str)     \
    do                    \
    {                     \
        std::cout << str; \
    } while (0);
#else
#define MM_ASSERT(expression) ((void)0);
#define MM_DEBUG_BREAK(expression) ((void)0);
#define MM_DEBUG_EXPRESSION(code) ((void)0);
#define MM_PRINT(str) ((void)0);
#endif

#pragma endregion

namespace MM
{
static constexpr size_t PAGE_SIZE = 4 * 1024;
static constexpr size_t SIZE_OF_LARGE_TYPE = 1024 * 4;
static constexpr size_t MIN_SLOT_SIZE = 64;

consteval size_t get_powered_size(size_t power)
{
    return size_t{1} << power;
}

constexpr uint32_t get_pow2_ceil(uint32_t number)
{
    uint32_t n = number;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

#pragma region Benchmark
//======================================================================
class Benchmark
{
public:
    Benchmark(float AvgPeriodSecs);
    ~Benchmark();

    void StartAlloc();
    void StartDealloc();
    void EndAlloc();
    void EndDealloc();
    void Print();
    void Tick(float dtTime);
    std::string FormatToSecMsMcs(std::chrono::microseconds timer);
    std::string FormatToSeconds(std::chrono::microseconds timer);

private:
    // Overall Allocations\Deallocations
    size_t m_NumAllocations;
    size_t m_NumDeallocations;

    // Overall Allocations\Deallocations time
    std::chrono::microseconds m_OverallAllocTime;
    std::chrono::microseconds m_OverallDeallocTime;

    // Longest Allocations\Deallocation time
    std::chrono::microseconds m_LongestAllocTime;
    std::chrono::microseconds m_LongestDeallocTime;

    // Average Allocations\Deallocations time Per Second
    std::chrono::microseconds m_AverageAllocTime;
    std::chrono::microseconds m_AverageDeallocTime;

    // Allocation\Deallocation time within one second
    std::chrono::microseconds m_FrameAllocTime;
    std::chrono::microseconds m_FrameDeallocTime;

    // Overall time
    std::chrono::microseconds m_OverallTime;

    std::chrono::time_point<std::chrono::steady_clock> m_StartAllocTimer;
    std::chrono::time_point<std::chrono::steady_clock> m_StartDeallocTimer;

    size_t m_FramesPerSecond;
    float m_FramesPerSecondTimer;

    float m_AverageTimer;

    float m_AveragePeriodTime;
    size_t m_FramesAverage;
    float m_AveragePeriod;
};
#pragma endregion

#pragma region Chunk
//======================================================================
struct Chunk
{
public:
    Chunk(unsigned char* ptr, uint32_t slotSize, uint8_t slotCount);
    ~Chunk();

    void SetNextChunkRecursive(Chunk* nextChunk);

    unsigned char* TryAcquireSlotInChunkChain();

    void FreeChunkSlot(unsigned char* ptr);

    void PrintBucket();

    void Print(uint32_t chunkNum);
    void ChunkSnapshotToStream(uint32_t chunkNum, std::ofstream& stream);
    void ChunkSnapshot(uint32_t chunkNum, char*& buffer);

    Chunk* IsPtrToBlockIsInChunkRecursive(const unsigned char* const);

private:
    const uint32_t m_SlotSize;
    const uint32_t m_CapacityBytes;
    const uint32_t m_SlotsCount;

    uint32_t m_UsedBytes;

    unsigned char* m_Head;
    unsigned char* m_Tail;
    Chunk* next;
    Fleur::Core::BitSet64 bitmap;

    struct PrintSlot
    {
        PrintSlot(uint32_t slotNum, bool isFree);

        void ToStream(std::string& upper, std::string& down);
        void ToStreanMinimalistic(std::string& string);

        bool m_IsFree;
        uint32_t m_Slot_num;
    };
};
#pragma endregion

#pragma region Pool
//======================================================================
struct Pool
{
    Pool(unsigned char* ptr, uint32_t slotSize, uint8_t slotCount);
    ~Pool();

    unsigned char* AcquireSlotFromPool();

    void Extend(Chunk* chunk);

    void Print();
    void PoolSpapshotToStream(std::ofstream& stream);
    void PoolSpapshot(char*& buffer);

    bool FreeSlot(unsigned char* ptr);

    bool IsInChunkChain(const unsigned char* const ptr) const;

private:
    Chunk* m_HeadChunk;
    uint32_t m_NumChunks;
    const uint32_t m_SlotSize;
    const uint8_t m_SlotsCount;
};
#pragma endregion

#pragma region Arena
//======================================================================
struct Arena
{
    Arena(unsigned char* ptr, size_t capacity, uint32_t pageSize, uint32_t minSlotSize);
    ~Arena();

    [[nodiscard]] Pool* GetPool(uint32_t slotSize);

    [[nodiscard]] Chunk* TryToGetNewChunk(Pool* pool, uint32_t slotSize, uint8_t slotsCount);

    Arena operator=(const Arena& other);

    void Print();

    void ArenaSnapshotToStream(std::ofstream& stream);
    void ArenaSnapshot(char* buffer);

private:
    const uint32_t m_PageSize;
    const uint32_t m_MinSlotSize;
    const size_t m_CapacityBytes;

    unsigned char* m_Head;

    // one-past-the-end
    unsigned char* m_Tail;

    unsigned char* m_Current;
    size_t m_UsedBytes;
};
#pragma endregion

#pragma region MemoryManager
//======================================================================
/*
 * @brief Memory Manager designed for Fleur Game Engine.
 * @details Manages memory arenas, poools, controls allocation\deallocation.
 * Core component.
 */
class MemoryManager
{
public:
    MemoryManager(size_t capacity, uint32_t arenaSize, uint32_t pageSize, uint8_t minSlotSize);
    ~MemoryManager();

    template <class T, size_t Align = 0>
        requires Fleur::Concepts::IsDefaultConstructible<T>
    [[nodiscard]] T* allocate(uint32_t count)
    {
        uint32_t sizeOfType = sizeof(T);
        assert(count > 0 && sizeOfType * count <= std::numeric_limits<uint32_t>::max());

        uint32_t requestedBytes = sizeOfType * count;
        uint32_t slotSize = CalculateSlotSize(requestedBytes);

        uint8_t slotsPerPage = m_PageSize / slotSize;

        bool isObjectLarge = slotSize > SIZE_OF_LARGE_TYPE;
        if (!isObjectLarge)
        {
            Pool* pool = nullptr;
            unsigned char* requestedMemory = nullptr;

            pool = m_LocalArena->GetPool(slotSize);

            if (pool)
            {
                requestedMemory = pool->AcquireSlotFromPool();
                if (!requestedMemory)
                {
                    // Not enought space in chunk
                    // Trying to get another chunk for that pool from Arena
                    auto newChunk = m_LocalArena->TryToGetNewChunk(pool, slotSize, slotsPerPage);
                    if (newChunk)
                    {
                        pool->Extend(newChunk);
                        requestedMemory = newChunk->TryAcquireSlotInChunkChain();
                    }
                    else
                    {
                        // TODO Not enought space in arena for new chunk, allocate new arena?
                        requestedMemory = nullptr;
                    }
                }
                if (requestedMemory)
                {
                    // placement new
                    MM_DEBUG_EXPRESSION({
                        T* ptr = nullptr;
                        ptr = new (requestedMemory) T;
                        MM_PRINT("Return ptr{" << static_cast<void*>(ptr) << "}\n")
                        return ptr;
                    })

                    return new (requestedMemory) T;
                }
                else
                {
                    // No space in pool, couldn't create new chunk
                    MM_DEBUG_BREAK(true);
                    return nullptr;
                }
            }
            else
            {
                // TODO couldn't wrong size data
                // Not enought space in arena? Create new arena?
                MM_DEBUG_BREAK(true);
                assert(false);
            }
        }
        else
        {
            // TODO think about large object strategy
            MM_DEBUG_BREAK(true);
        }
    }

    template <class T>
    void deallocate(void* ptr, uint32_t num)
    {
        if (!ptr)
            return;

        uint32_t blockSizeBytes = sizeof(T) * num;
        assert(blockSizeBytes <= std::numeric_limits<uint32_t>::max());

        uint32_t alignedBlockSize = get_pow2_ceil(blockSizeBytes);

        unsigned char* bytePtr = reinterpret_cast<unsigned char*>(ptr);
        const bool isObjectLarge = blockSizeBytes >= SIZE_OF_LARGE_TYPE;
        if (!isObjectLarge)
        {
            auto pool = m_LocalArena->GetPool(alignedBlockSize);
            if (pool)
            {
                bool res = pool->FreeSlot(bytePtr);
                if (!res)
                    __debugbreak();
                else
                {
                    // placement new deallocation
                    reinterpret_cast<T*>(bytePtr)->~T();
                }
            }
        }
    }

    void Print();

    void SaveSnapshotToFile(std::string_view fileName, uint32_t iteration);

    void ClearFile(std::string_view fileName);

private:
    unsigned char* m_Head;
    Arena* m_LocalArena;

    const size_t m_Capacity;
    uint32_t m_UsedBytes;

    const uint32_t m_PageSize;
    const uint8_t m_MinSlotSize;

    const uint32_t m_ArenaSize;

    uint32_t CalculateSlotSize(uint32_t original);
};
#pragma endregion

}  // namespace MM