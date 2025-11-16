#pragma once

#include <fstream>
#include <limits>

#include "../Engine/Fleur/Concepts.hpp"

#pragma region MemoryManager Debug profiling definitions
//======================================================================
#define MEMORYMANAGER_PROFILING
#ifdef _DEBUG&& defined MEMORYMANAGER_PROFILING
#include <algorithm>

struct MemoryInfo
{
    struct record
    {
        record(uint32_t slotSize)
            : size(slotSize)
            , allocAmount(0)
            , deallocAmount(0)
            , overallMemoryBytes(0) {};

        uint32_t size;
        size_t allocAmount;
        size_t deallocAmount;
        size_t overallMemoryBytes;

        inline bool operator<(const record& other) const
        {
            return this->overallMemoryBytes < other.overallMemoryBytes;
        }
        inline bool operator>(const record& other) const
        {
            return this->overallMemoryBytes > other.overallMemoryBytes;
        }
        inline bool operator<=(const record& other)
        {
            return !(this->operator>(other));
        }
        inline bool operator>=(const record other)
        {
            return !(this->operator<(other));
        }
        inline bool operator==(const record& other)
        {
            return this->allocAmount == other.allocAmount;
        }
        inline bool operator!=(const record& other)
        {
            return !(this->operator==(other));
        }
    };

    MemoryInfo() = default;


    std::unordered_map<uint32_t, record> infos;

    void AddAlloc(size_t slotSize);
    void AddDealloc(size_t slotSize);
    static inline std::string FormatBytes(size_t bytes);

    void Print() const;
};

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

#define MM_PRINT(str) \
    //do                    \
    //{                     \
    //    std::cout << str; \
    //} while (0);


#else
#define MM_ASSERT(expression) ((void)0);
#define MM_DEBUG_BREAK(expression) ((void)0);
#define MM_DEBUG_EXPRESSION(code) ((void)0);
#define MM_PRINT(str) ((void)0);
#endif

#pragma endregion

#define II_NULL_INDEX 0xffffffff
#define INVALID_OFFSET 0xFFFFFFFF
#define LOCAL_HEAD(ptr, type) unsigned char* localHead = reinterpret_cast<unsigned char*>(ptr) + sizeof(type)
#define TOCHARPTR(ptr) reinterpret_cast<unsigned char*>(ptr)
#define CHUNK_STRIDE (sizeof(Chunk) + 4096)
namespace MM
{
static constexpr size_t PAGE_SIZE = 4 * 1024;
static constexpr size_t SIZE_OF_LARGE_TYPE = 1024 * 4;
static constexpr size_t MIN_SLOT_SIZE = 16;

#pragma region Bits
consteval size_t get_powered_size(size_t power)
{
    return 1ull << power;
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

inline uint8_t GetPowerOfTwoOf(uint32_t number)
{
    if (number == 0)
        return 0;

    uint8_t power = 0;
    while (1u << power <= number)
    {
        power++;
    }
    return power;
}

inline size_t AlignTo(size_t size, size_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

inline uint32_t CountSlots(uint32_t size, uint32_t slotSize = 8)
{
    return (size + slotSize - 1) / slotSize;
}
#pragma endregion

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
    Chunk(unsigned char* ptr, uint32_t slotSize, uint32_t slotCount);
    ~Chunk();

    // True - there is at least one free chunk after acquisition
    // False - Chunk is full
    bool AcquireSlot(unsigned char*& outPtr);

    // True - Pool must add this chunk to free list
    // False - This chunk is already in a free list
    bool FreeChunkSlot(unsigned char* ptrToSLot, bool* isEmpty);

    void PrintBucket();

    void Print(uint32_t chunkNum);
    void ChunkSnapshotToStream(uint32_t chunkNum, std::ofstream& stream);
    void ChunkSnapshot(uint32_t chunkNum, char*& buffer);

    inline bool IsValid() const
    {
        return m_UsedBytes < 5000;
    }

    void SetNext(Chunk* ptr);
    void SetPrev(Chunk* prev);
    inline Chunk* GetNext()
    {
        if (m_NextChunkOffsetInChunkStride == 0)
            return nullptr;

        Chunk* chunk = reinterpret_cast<Chunk*>(TOCHARPTR(this) + (m_NextChunkOffsetInChunkStride * static_cast<int>(CHUNK_STRIDE)));
        MM_DEBUG_BREAK(!chunk->IsValid());
        return chunk;
    }
    inline Chunk* GetPrev()
    {
        if (m_PrevChunkOffsetInChunkStride == 0)
            return nullptr;

        Chunk* chunk = reinterpret_cast<Chunk*>(TOCHARPTR(this) + (m_PrevChunkOffsetInChunkStride * static_cast<int>(CHUNK_STRIDE)));
        MM_DEBUG_BREAK(!chunk->IsValid());
        return chunk;
    }

private:
    const uint32_t m_SlotSize;
    const uint32_t m_CapacityBytes;

    uint32_t m_UsedBytes;

    uint32_t m_FreeSlot;

    // Offset: n * (sizeof(Chunk) + PageSize) bytes
    int m_NextChunkOffsetInChunkStride;
    int m_PrevChunkOffsetInChunkStride;

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
    Pool(uint32_t slotSize, uint32_t slotCount);
    ~Pool();

    // False - no space
    bool AcquireSlotFromPool(unsigned char*& outPtr);

    void Extend(Chunk* chunk);

    void Print();
    void PoolSpapshotToStream(std::ofstream& stream);
    void PoolSpapshot(char*& buffer);

    // True - Chunk is empty
    bool FreeSlot(Chunk* chunk, unsigned char* ptrToSlot);

private:
    uint32_t m_HeadOffsetBytes;
    uint32_t m_NumChunks;
    const uint32_t m_SlotSize;
    const uint32_t m_SlotsCount;
};
#pragma endregion

#pragma region Arena
//======================================================================
struct Arena
{
    Arena(unsigned char* ptr, size_t capacity, uint32_t pageSize, uint32_t minSlotSize);
    ~Arena();

    [[nodiscard]] Pool* GetPool(uint32_t slotSize);
    [[nodiscard]] Chunk* FindChunk(unsigned char* ptrToSlot);

    [[nodiscard]] Chunk* TryToGetNewChunk(Pool* pool, uint32_t slotSize, uint32_t slotsCount);

    Arena operator=(const Arena& other);

    void Print();

    void ArenaSnapshotToStream(std::ofstream& stream);
    void ArenaSnapshot(char* buffer);

    const uint32_t m_PageSize;
    const uint32_t m_MinSlotSize;
    const size_t m_CapacityBytes;
    uint32_t m_StaticOffset;
    unsigned char* m_Head;

    // one-past-the-end
    unsigned char* m_Tail;

    unsigned char* m_Current;
    void* m_SmallObjectsCurrent;
    size_t m_UsedBytes;
};
#pragma endregion

#pragma region MemoryManager
//======================================================================
/*
 * @brief Memory Manager designed for Fleur Game Engine.
 * @details Manages memory arenas, poools, controls allocation\deallocation.
 * Core component.
 * II: Internal Index: points to the next free slot
 * Range: [0; UINT32_MAX], where UINT32_MAX means "no next free slot"
 */
class MemoryManager
{
public:
    MemoryManager(size_t capacity, uint32_t arenaSize);
    ~MemoryManager();

    template <class T, size_t Align = 0>
        requires Fleur::Concepts::IsDefaultConstructible<T>
    [[nodiscard]] T* allocate(uint32_t count)
    {
        assert(count > 0 && sizeof(T) * count <= std::numeric_limits<uint32_t>::max());

        uint32_t slotSize = AlignTo(sizeof(T) * count, 8);

        bool isObjectLarge = slotSize > SIZE_OF_LARGE_TYPE / 2;
        if (!isObjectLarge)
        {
            Pool* pool = nullptr;
            unsigned char* requestedMemoryPtr = nullptr;

            pool = m_LocalArena->GetPool(slotSize);

            if (pool)
            {
                // Check first if Arena already has Chunk in free list:
                if (m_LocalArena->m_SmallObjectsCurrent)
                {
                    void* next = *reinterpret_cast<void**>(m_LocalArena->m_SmallObjectsCurrent);
                    unsigned char* buffer = TOCHARPTR(m_LocalArena->m_SmallObjectsCurrent);
                    Chunk* chunk = new (buffer) Chunk(buffer + sizeof(Chunk), slotSize, m_PageSize / slotSize);
                    pool->Extend(chunk);

                    m_LocalArena->m_SmallObjectsCurrent = next;
                }
                bool res = pool->AcquireSlotFromPool(requestedMemoryPtr);
                if (!res)
                {
                    uint32_t slotsPerPage = m_PageSize / slotSize;
                    auto newChunk = m_LocalArena->TryToGetNewChunk(pool, slotSize, slotsPerPage);
                    if (newChunk)
                    {
                        pool->Extend(newChunk);
                        pool->AcquireSlotFromPool(requestedMemoryPtr);
                    }
                }

                if (requestedMemoryPtr)
                {
                    // placement new
                    if (count > 1)
                    {
                        // Array
                        T* arrayPtr = reinterpret_cast<T*>(requestedMemoryPtr);

                        for (size_t i = 0; i < count; i++)
                        {
                            new (arrayPtr + i) T;
                        }

                        return arrayPtr;
                    }
                    else
                        return new (requestedMemoryPtr) T;
                }
                else
                {
                    // No space in Arena for new chunk
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
    void deallocate(void* ptr, uint32_t count)
    {
        if (!ptr)
            return;

        assert(sizeof(T) * count <= std::numeric_limits<uint32_t>::max());

        uint32_t alignedBlockSize = AlignTo(sizeof(T) * count, 8);
        MM_DEBUG_BREAK(alignedBlockSize > m_PageSize);

        if (!(alignedBlockSize >= SIZE_OF_LARGE_TYPE))
        {
            unsigned char* bytePtr = reinterpret_cast<unsigned char*>(ptr);
            auto pool = m_LocalArena->GetPool(alignedBlockSize);
            if (pool)
            {
                Chunk* chunk = m_LocalArena->FindChunk(bytePtr);
                bool isChunkEmpty = pool->FreeSlot(chunk, bytePtr);

                if (isChunkEmpty)
                {
                    // Check if this chunk is latest allocated chunk
                    if (TOCHARPTR(chunk) == (m_LocalArena->m_Current - (sizeof(Chunk) + m_PageSize)))
                    {
                        // This is latest allocated chunk, we don't want to store it
                        uint32_t usedBytes = sizeof(Chunk) + m_PageSize;
                        m_LocalArena->m_Current -= usedBytes;
                        m_LocalArena->m_UsedBytes -= usedBytes;
                    }
                    else
                    {
                        if (!m_LocalArena->m_SmallObjectsCurrent)
                        {
                            m_LocalArena->m_SmallObjectsCurrent = reinterpret_cast<void*>(chunk);
                            *reinterpret_cast<void**>(m_LocalArena->m_SmallObjectsCurrent) = nullptr;
                        }
                        else
                        {
                            void* current = m_LocalArena->m_SmallObjectsCurrent;
                            m_LocalArena->m_SmallObjectsCurrent = reinterpret_cast<void*>(chunk);
                            *reinterpret_cast<void**>(chunk) = current;
                        }
                    }
                }
                // placement new deallocation
                if (count > 1)
                {
                    T* arrayPtr = reinterpret_cast<T*>(bytePtr);
                    for (size_t i = 0; i < count; i++)
                    {
                        (arrayPtr + i)->~T();
                    }
                }
                else
                {
                    reinterpret_cast<T*>(bytePtr)->~T();
                }
            }
        }
    }

    void Print();
    std::string GetSnapshot() const;

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