#pragma once

#include <fstream>
#include <limits>

#include "../Engine/Fleur/Concepts.hpp"
#include "DebugDefenitions.hpp"
#include "PageAllocator.hpp"
#include "SLUBAllocator.hpp"
namespace MM
{
static constexpr size_t PAGE_SIZE = 4 * 1024;
static constexpr size_t SIZE_OF_LARGE_TYPE = 1024 * 4;
static constexpr size_t MIN_SLOT_SIZE = 16;

#pragma region Utils
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

#pragma region Arena
//======================================================================
struct Arena
{
    Arena operator=(const Arena& other);

    void Print();

    void ArenaSnapshotToStream(std::ofstream& stream);
    void ArenaSnapshot(char* buffer);
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
    ~MemoryManager();

    static MemoryManager* ManagerFabric(size_t capacity);

    /**
     * @brief Requests allocation and construction of a single object or an array of objects.
     *
     * @tparam T           Object type to construct.
     * @tparam Align       Optional alignment override (0 = use allocator's default).
     * @param count        Number of objects to allocate (1 = single object).
     *
     * @return Pointer to constructed object(s), or nullptr if allocation fails.
     *
     * @details
     * This function does NOT perform allocation directly. Instead, it delegates the
     * request to the appropriate allocator selected by the MemoryManager.
     *
     * Allocation flow:
     *  1. Compute required slot size based on sizeof(T), count, and alignment.
     *  2. MemoryManager determines which allocator (Slub, Pool, etc.) is responsible
     *     for handling this allocation size.
     *  3. The chosen allocator performs all allocation logic internally
     *     (chunk reuse, free-list lookup, chunk creation, etc.).
     *  4. Once raw memory is provided, MemoryManager uses placement-new
     *     to construct T (for arrays, constructs each element).
     *
     * Notes:
     *  - Large allocations (`slotSize >= SIZE_OF_LARGE_TYPE`) are still forwarded to
     *    the appropriate allocator, but support may be limited depending on the allocator.
     *  - Returns nullptr if the allocator fails to produce a valid memory block.
     */
    template <class T, size_t Align = 0>
        requires Fleur::Concepts::IsDefaultConstructible<T>
    [[nodiscard]] T* allocate(uint32_t count)
    {
        assert(count > 0 && sizeof(T) * count <= std::numeric_limits<uint32_t>::max());

        uint32_t slotSize = AlignTo(sizeof(T) * count, 8);

        bool isObjectLarge = slotSize > SIZE_OF_LARGE_TYPE / 2;

        if (slotSize <= 2048)
        {
            return slubAlloc->allocate<T, Align>(slotSize, count);
        }
        else if (slotSize > 2048 && slotSize < 64'000)
        {
            // TODO: Two-Level Segregated Fit memory allocator
            MM_DEBUG_BREAK(true);
        }
        else
        {
            // TODO: VirtualAlloc
            MM_DEBUG_BREAK(true);
        }
    }

    /**
     * @brief Allocates raw memory for a single object or an array of objects of type T without calling constructors.
     * @tparam T Object type.
     * @tparam Align Optional alignment override (default = auto-align).
     * @param count Number of objects to allocate.
     * @return Pointer to uninitialized memory suitable for placement new, or nullptr on failure.
     *
     * @details
     * This method is useful for:
     * - manually constructing objects later with placement new,
     * - allocating POD arrays or buffers,
     * - optimizing batch allocations.
     *
     * The caller is responsible for invoking constructors and destructors.
     */
    template <class T, size_t Align = 0>
    [[nodiscard]] T* allocate_raw(uint32_t count)
    {
        // TODO
    }

    /**
     * @brief Destroys and frees a single object or array previously allocated.
     *
     * @tparam T       Object type.
     * @param ptr      Pointer to object or array.
     * @param count    Number of objects that were allocated.
     *
     * @details
     * Deallocation process:
     *  1. Call destructors via explicit destructor invocation.
     *  2. Locate the pool and owning chunk.
     *  3. Return the slot back to the pool.
     *  4. If the chunk becomes empty:
     *      - If it was the most recent one, arena rewinds its pointer.
     *      - Otherwise, chunk goes into the recycle list.
     *
     * Large objects (`slotSize >= SIZE_OF_LARGE_TYPE`) are not yet supported.
     */
    template <class T>
    void deallocate(void* ptr, uint32_t count)
    {
        if (!ptr)
            return;

        assert(sizeof(T) * count <= std::numeric_limits<uint32_t>::max());

        uint32_t slotSize = AlignTo(sizeof(T) * count, 8);
        MM_DEBUG_BREAK(slotSize > m_PageSize);

        if (slotSize <= 2048)
        {
            slubAlloc->deallocate<T>(ptr, slotSize, count);
        }
        else if (slotSize > 2048 && slotSize < 64'000)
        {
            // TODO: Two-Level Segregated Fit memory allocator
            MM_DEBUG_BREAK(true);
        }
        else
        {
            // TODO: VirtualAlloc
            MM_DEBUG_BREAK(true);
        }
    }

    /**
     * @brief Frees memory previously allocated for a single object or an array of objects of type T and calls destructors.
     * @tparam T Object type.
     * @param ptr Pointer to memory allocated by allocate or allocate_raw.
     * @param count Number of objects to free.
     *
     * @details
     * Deallocation process:
     * 1. Calls destructors for all objects in the array.
     * 2. Returns slots to the corresponding pool.
     * 3. If the chunk becomes empty:
     *    - If it was the most recent chunk, the arena rewinds its pointer.
     *    - Otherwise, the chunk is added to the recycle list.
     *
     * Large objects (>= SIZE_OF_LARGE_TYPE) are not yet supported.
     */
    template <class T>
    void free(void* ptr, uint32_t count)
    {
        // TODO
    }

    /**
     * @brief Prints debug info about memory usage, pools and chunks.
     */
    void Print();

    /**
     * @brief Returns a text snapshot of internal memory state.
     *
     * @return String containing human-readable debug information.
     */
    std::string GetSnapshot() const;

private:
    /**
     * @brief Creates a new MemoryManager with a fixed memory capacity.
     *
     * @param capacity    Total memory in bytes to reserve for the arena.
     * @param arenaSize   Size of a single arena in bytes.
     *
     * @note The entire memory region is pre-allocated and never grows.
     */
    unsigned char* m_Head;

    const size_t m_Capacity;
    uint32_t m_UsedBytes;

    const uint32_t m_PageSize;
    const uint8_t m_MinSlotSize;

    PageAllocator* pageAlloc;
    SLUBAllocator* slubAlloc;

    MemoryManager(unsigned char* ptr, size_t capacity);
    uint32_t CalculateSlotSize(uint32_t original);
};
#pragma endregion

}  // namespace MM