#pragma once

#include <fstream>
#include <iostream>
#include <limits>

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
struct Benchmark
{
    static size_t m_NumAllocations;
    static size_t m_NumDeallocations;
    static std::chrono::microseconds m_LongestAllocTime;
    static std::chrono::microseconds m_AverageAllocTime;
    static std::chrono::microseconds m_SumAllocTime;
    static std::chrono::microseconds m_FrameAllocTime;
    static size_t frames;
    std::chrono::time_point<std::chrono::steady_clock> start;

public:
    Benchmark() = default;
    ~Benchmark() = default;

    void Start();
    void End();
    static void Print();
    inline void Deallocate()
    {
        ++m_NumDeallocations;
    }
    static void Frame()
    {
        ++frames;
    }
    static void EndOfFrame();
    static std::string FormatToSecMsMcs(std::chrono::microseconds timer);
};
#pragma endregion

#pragma region Bits

uint64_t bit_set(uint64_t number, uint8_t n) noexcept;

uint64_t bit_clear(uint64_t number, uint8_t n) noexcept;

uint64_t bit_toggle(uint64_t number, uint8_t n) noexcept;

// True     = 1 = Occupied
// False    = 0 = Free
bool bit_check(uint64_t number, uint8_t n) noexcept;

#pragma endregion

#pragma region BitSet64
//======================================================================
/*
 * @brief General purpose bits container, stores up to 64 flags. 0 - free, 1 - occupied
 * @details Bits operations sucs as set\check\clear\toggle.
 */
class BitSet64
{
public:
    BitSet64(uint8_t bits)
        : m_Bitmap(0)
        , m_Bits(bits)
    {
        assert(m_Bits <= 64, "Type is too large (>64 bytes)");
        assert(m_Bits > 0, "Bits must be more than 0");

        m_Bitmap = 0;
    }

    bool IsBitOccupied(uint8_t idx) const
    {
        return bit_check(m_Bitmap, idx);
    }

    void SetBit(uint8_t idx)
    {
        if (idx > m_Bits - 1)
            __debugbreak();

        m_Bitmap = bit_set(m_Bitmap, idx);
    }

    void ClearBit(uint8_t idx)
    {
        m_Bitmap = bit_clear(m_Bitmap, idx);
    }

    void ToggleBit(uint8_t idx)
    {
        m_Bitmap = bit_toggle(m_Bitmap, idx);
    }

    bool IsFull() const
    {
        uint64_t mask = ~0ull >> (64 - m_Bits);
        return m_Bitmap == mask;
    }

private:
    uint64_t m_Bitmap;
    uint8_t m_Bits;
};
#pragma endregion

#pragma region Chunk
//======================================================================
struct Chunk
{
public:
    Chunk(unsigned char* ptr, uint32_t slotSize, uint8_t slotCount);
    ~Chunk() = default;

    void SetNextChunkRecursive(Chunk* nextChunk);

    unsigned char* TryAcquireSlotInChunkChain();

    void FreeChunkSlot(unsigned char* ptr);

    void PrintBucket();

    void Print(uint32_t chunkNum);
    void ChunkSnapshotToStream(uint32_t chunkNum, std::ofstream& stream);

    Chunk* IsPtrToBlockIsInChunkRecursive(unsigned char* ptr);

private:
    const uint32_t m_SlotSize;
    const uint32_t m_CapacityBytes;
    const uint32_t m_SlotsCount;

    uint32_t m_UsedBytes;

    unsigned char* m_Head;
    unsigned char* m_Tail;
    Chunk* next;
    BitSet64 bitmap;
};
#pragma endregion

#pragma region Pool
//======================================================================
struct Pool
{
    Pool(unsigned char* ptr, uint32_t slotSize, uint8_t slotCount);
    ~Pool() = default;

    unsigned char* AcquireSlotFromPool();

    void Extend(Chunk* chunk);

    void Print();
    void PoolSpapshotToStream(std::ofstream& stream);

    bool FreeSlot(unsigned char* ptr);

private:
    Chunk* m_HeadChunk;
    uint32_t m_NumChunks;
    const uint32_t m_SlotSize;
    const uint32_t m_SlotsCount;
};
#pragma endregion

#pragma region Arena
//======================================================================
struct Arena
{
    Arena(size_t capacity, size_t pageSize, uint32_t minSlotSize);
    ~Arena()
    {
        Free();
    }
    void Free();

    [[nodiscard]] Pool* CreatePool(uint32_t slotSize);
    [[nodiscard]] Pool* GetPool(uint32_t slotSize);

    [[nodiscard]] Chunk* TryToGetNewChunk(Pool* pool, uint32_t slotSize, uint8_t slotsCount);

    Arena operator=(const Arena& other);

    void Print();

    void ArenaSnapshotToStream(std::ofstream& stream);


private:
    const uint32_t m_PageSize;
    const uint32_t m_MinSlotSize;
    const size_t m_CapacityBytes;

    unsigned char* m_Head;

    // one-past-the-end
    unsigned char* m_Tail;

    unsigned char* m_Current;
    size_t m_UsedBytes;

    std::unordered_map<uint32_t, void*> map;
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
    [[nodiscard]] T* allocate(uint32_t count)
    {
        uint32_t sizeOfType = sizeof(T);
        assert(count > 0 && sizeOfType * count <= std::numeric_limits<uint32_t>::max(), "Allocation size must be > 0");

        uint32_t requestedBytes = sizeOfType * count;
        uint32_t slotSize = CalculateSlotSize(requestedBytes);

        uint8_t slotsPerPage = m_PageSize / slotSize;

        bool isObjectLarge = slotSize > SIZE_OF_LARGE_TYPE;
        if (!isObjectLarge)
        {
            Pool* pool = nullptr;
            unsigned char* requestedMemory = nullptr;

            pool = m_LocalArena->GetPool(slotSize);
            if (!pool)
                pool = m_LocalArena->CreatePool(slotSize);

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
                    return new (requestedMemory) T;
                }
                else
                {
                    // No space in pool, couldn't create new chunk
                    __debugbreak();
                    return nullptr;
                }
            }
            else
            {
                // TODO couldn't create new Pool
                // Not enought space in arena? Create new arena?
                assert(false);
            }
        }
        else
        {
            // TODO think about large object strategy
            __debugbreak();
        }
    }

    template <class T>
    void deallocate(void* ptr, uint32_t num)
    {
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

    void SaveSnapshotToFile(std::string_view fileName);

    void ClearFile(std::string_view fileName);

private:
    const uint32_t m_ArenaSize;
    const uint32_t m_PageSize;
    const size_t m_Capacity;
    const uint8_t m_MinSlotSize;
    Arena* m_LocalArena;

    uint32_t CalculateSlotSize(uint32_t original);
};
#pragma endregion

}  // namespace MM