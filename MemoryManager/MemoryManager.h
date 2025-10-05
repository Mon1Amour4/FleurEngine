#pragma once
#include <limits>

namespace MM
{
static constexpr size_t PAGE_SIZE = 4 * 1024;
static constexpr size_t SIZE_OF_LARGE_TYPE = 1024 * 1;
static constexpr size_t MIN_SLOT_SIZE = 64;

consteval size_t get_powered_size(size_t power)
{
    return size_t{1} << power;
}

template <uint32_t Number>
constexpr uint32_t get_pow2_ceil()
{
    uint32_t n = Number;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

bool IsWithinPool(uint32_t used, uint32_t capacity, uint32_t allocationSize)
{
    return used + allocationSize <= capacity;
}

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

template <class T>
class BitSet
{
public:
    BitSet()
    {
        bitmap = 0;
    }

    bool CheckBit(uint32_t idx)
    {
    }
    void SetBit(uint32_t idx, bool flag)
    {
    }

private:
    T bitmap;
};

// template <typename T>
// struct CustomAllocator
//{
//     using value_type = T;
//
//     constexpr CustomAllocator() noexcept
//     {
//         mark = Benchmark();
//     }
//     constexpr ~CustomAllocator() = default;
//     template <class U>
//     constexpr CustomAllocator(const CustomAllocator<U>&) noexcept
//     {
//     }
//     [[nodiscard]] constexpr T* allocate(size_t n)
//     {
//         FL_CORE_INFO("[ALLOCATOR] Allocated {0} bytes for {1} of type", n * sizeof(T), n);
//         mark.Start();
//         T* ptr = static_cast<T*>(malloc(n * sizeof(T)));
//         mark.End();
//         return ptr;
//     }
//     constexpr void deallocate(T* p, size_t n)
//     {
//         FL_CORE_INFO("[ALLOCATOR] Deallocated {0} bytes for {1} of type", n * sizeof(T), n);
//         free(p);
//
//         mark.Deallocate();
//     }
//     bool operator==(const CustomAllocator&) const noexcept
//     {
//         return true;
//     }
//     bool operator!=(const CustomAllocator&) const noexcept
//     {
//         return false;
//     }
//     Benchmark mark;
// };

// class FreeBlock
//{
// public:
//     FreeBlock(unsigned char* ptr, uint32_t size);
//     ~FreeBlock() = default;
//
//     auto operator<=>(const FreeBlock& other) const;
//
//     void RemoveBlock();
//
//     void SetNext(FreeBlock* next);
//
// private:
//     unsigned char* m_Ptr;
//     uint32_t m_Size;
//     FreeBlock* m_Next;
//     FreeBlock* m_Prev;
// };

template <size_t SlotSize, size_t NumSlots>
struct Chunk
{
public:
    Chunk(unsigned char* ptr)
        : m_CapacityBytes(SlotSize * NumSlots)
        , m_UsedBytes(0)
        , m_Head(nullptr)
        , m_Tail(nullptr)
        , m_Current(nullptr)
        , next(nullptr)
        , bitmap()
    {
        // Chunk size must not exceed uint32_t size
        assert(m_CapacityBytes <= std::numeric_limits<uint32_t>::max());

        std::cout << "Chunk{" << SlotSize << "," << NumSlots << "} has been created\n";

        m_Current = m_Head = ptr;
        m_Tail = m_Head + m_CapacityBytes;
    }

    void SetNextChunkRecursive(Chunk<SlotSize, NumSlots>* nextChunk)
    {
        assert(nextChunk);

        if (!next)
            next = nextChunk;
        else
            next->SetNextChunkRecursive(nextChunk);
    }

    unsigned char* RequestSlotRecursive()
    {
        if (IsWithinPool(m_UsedBytes, m_CapacityBytes, SlotSize))
        {
            unsigned char* prevPtr = m_Current;
            m_Current += SlotSize;
            m_UsedBytes += SlotSize;
            return prevPtr;
        }

        if (next)
        {
            next->RequestSlotRecursive();
        }

        return nullptr;
    }

    bool deallocate(unsigned char* ptr, uint32_t num)
    {
        uint32_t requestedSize = SlotSize * num;
        unsigned char* blockEndPtr = ptr + requestedSize;
        if (blockEndPtr < m_Current)
        {
            m_UsedBytes -= requestedSize;
            m_Current = blockEndPtr;

            std::ptrdiff_t diff = blockEndPtr - m_Head;
            bitmap.SetBit(diff, true);

            return true;
        }
        else if (blockEndPtr == m_Current)
        {
            unsigned char* deallocatedPtr = m_Current - (SlotSize * num);
            if (deallocatedPtr >= m_Head)
            {
                m_Current = deallocatedPtr;
                m_UsedBytes -= requestedSize;

                std::ptrdiff_t diff = blockEndPtr - m_Head;
                bitmap.SetBit(diff, true);

                return true;
            }
            else
            {
                return false;
                // TODO Handle this case;
            }
        }
        return false;
    }

    void PrintBucket()
    {
        const char emptyCell = '-';
        const char occupiedCell = 'x';
        const int cellsPerRow = 10;

        const uint32_t numCells = m_CapacityBytes / SlotSize;

        std::cout << "\nBucket View (" << m_UsedBytes << "/" << m_CapacityBytes << ")\n";

        uint32_t filled = m_UsedBytes / SlotSize;
        for (uint32_t i = 0; i < numCells; i += cellsPerRow)
        {
            std::cout << "\\";
            for (uint32_t j = 0; j < cellsPerRow; j++)
            {
                uint32_t index = i + j;
                if (index < filled)
                    std::cout << occupiedCell;
                else
                    std::cout << emptyCell;
            }
            std::cout << "/\n";
        }
    }

    void Print(uint32_t chunkNum)
    {
        static constexpr char emptyCell = ' ';
        static constexpr char occupiedCell = 'x';
        static constexpr uint32_t cellsPerRow = 40;

        std::cout << "\nChunk_" << chunkNum << "{" << SlotSize << ", " << NumSlots << "}: " << m_UsedBytes << "/" << m_CapacityBytes << "\n";

        const uint32_t numCells = m_CapacityBytes / SlotSize;

        uint32_t maxIndex = numCells ? numCells - 1 : 0;
        int width = (maxIndex > 0) ? static_cast<int>(std::log10(maxIndex)) + 1 : 1;

        uint32_t numRows = (numCells + cellsPerRow - 1) / cellsPerRow;

        for (uint32_t row = 0; row < numRows; row++)
        {
            uint32_t start = row * cellsPerRow;
            uint32_t end = std::min(start + cellsPerRow, numCells);

            for (uint32_t i = start; i < end; i++) std::cout << '[' << std::setw(width) << i << "] ";
            std::cout << "\n";

            for (uint32_t i = start; i < end; i++)
            {
                unsigned char* ptr = m_Head + (i * SlotSize);
                std::cout << "[ " << (ptr < m_Current ? occupiedCell : emptyCell) << "] ";
            }
            std::cout << "\n\n";
        }
        PrintBucket();

        if (next)
            next->Print(chunkNum++);
    }

    unsigned char* Tail() const
    {
        return m_Tail;
    }

    Chunk<SlotSize, NumSlots>* IsPtrToBlockIsInChunkRecursive(unsigned char* ptr)
    {
        if (ptr < m_Tail && ptr >= m_Head)
            return this;

        if (next)
            return next->IsPtrToBlockIsInChunkRecursive(ptr);
        else
            return nullptr;
    }

private:
    const uint32_t m_CapacityBytes;
    uint32_t m_UsedBytes;
    unsigned char* m_Head;
    unsigned char* m_Tail;
    unsigned char* m_Current;
    Chunk<SlotSize, NumSlots>* next;
    BitSet<uint64_t> bitmap;
};

template <uint32_t SlotSize, uint32_t SlotNum>
struct Pool
{
    Pool(unsigned char* ptr)
        : m_NumChunks(0)
        , m_HeadChunk(nullptr)
    {
        std::cout << "Pool{" << SlotSize << "," << SlotNum << "} has been created\n";

        m_HeadChunk = new Chunk<SlotSize, SlotNum>(ptr);

        ++m_NumChunks;
    }

    unsigned char* RequestSlot()
    {
        return m_HeadChunk->RequestSlotRecursive();
    }

    void Extend(Chunk<SlotSize, SlotNum>* chunk)
    {
        m_HeadChunk->SetNextChunkRecursive(chunk);
        m_NumChunks++;
    }

    void Print()
    {
        std::cout << "\nPrinting Pool{" << SlotSize << "," << SlotNum << "}: Chunks: " << std::to_string(m_NumChunks);
        m_HeadChunk->Print(0);
    }

    Chunk<SlotSize, SlotNum>* GetChunkByPtrToBlock(unsigned char* ptr)
    {
        return m_HeadChunk->IsPtrToBlockIsInChunkRecursive(ptr);
    }

private:
    Chunk<SlotSize, SlotNum>* m_HeadChunk;
    uint32_t m_NumChunks;
};

template <unsigned int Size>
struct Arena
{
    Arena()
    {
        m_CapacityBytes = Size;
        m_Head = static_cast<unsigned char*>(malloc(m_CapacityBytes));
        memset(m_Head, 0, m_CapacityBytes);

        assert(m_Head);

        m_Current = m_Head;
        m_Tail = m_Head + (m_CapacityBytes);
    }

    template <size_t SlotSize, size_t SlotsNum>
    [[nodiscard]] constexpr Chunk<SlotSize, SlotsNum>* TryToGetNewChunk(Pool<SlotSize, SlotsNum>* pool)
    {
        unsigned char* requestedPtr = m_Current + SlotSize * SlotsNum;
        if (requestedPtr < m_Tail)
        {
            // Arena has enought space for new chunk, give it
            unsigned char* prevPtr = m_Current;
            m_Current = requestedPtr;
            return new Chunk<SlotSize, SlotsNum>(prevPtr);
        }
        else
        {
            return nullptr;
        }
    }

    template <uint32_t SlotSize, uint32_t SlotsNum = PAGE_SIZE / SlotSize>
    [[nodiscard]] constexpr Pool<SlotSize, SlotsNum>* GetPool()
    {
        if (auto val = map.find(SlotSize); val != map.end())
            return static_cast<Pool<SlotSize, SlotsNum>*>(val->second);

        return nullptr;
    }

    template <uint32_t SlotSize, uint32_t SlotsNum = PAGE_SIZE / SlotSize>
    [[nodiscard]] constexpr Pool<SlotSize, SlotsNum>* CreatePool()
    {
        auto pool = GetPool<SlotSize, SlotsNum>();
        if (pool)
            return pool;

        // Test if we have enought capacity
        size_t updatedUsed = m_UsedBytes + SlotSize;
        if (updatedUsed <= m_CapacityBytes)
        {
            map[SlotSize] = new Pool<SlotSize, SlotsNum>(m_Current);
            m_Current += SlotSize;
            return static_cast<Pool<SlotSize, SlotsNum>*>(map[SlotSize]);
        }
        else
        {
            // Not enought space in current arena
            return nullptr;
        }
    }

    Arena<Size> operator=(const Arena& other)
    {
        return Arena<Size>();
    };

    void Free()
    {
        delete m_Head;
    }

    void Print()
    {
        if (auto val = map.find(256); val != map.end())
            static_cast<Pool<256, PAGE_SIZE / 256>*>(val->second)->Print();
        if (auto val = map.find(512); val != map.end())
            static_cast<Pool<512, PAGE_SIZE / 512>*>(val->second)->Print();
    }

private:
    unsigned char* m_Head;

    // one-past-the-end
    unsigned char* m_Tail;

    unsigned char* m_Current;
    size_t m_CapacityBytes, m_UsedBytes = 0;

    std::unordered_map<uint32_t, void*> map;
};

template <size_t Size>
struct MemoryManager
{
    MemoryManager()
    {
        static_assert(Size > 0, "Allocation size must be > 0");

        m_LocalArena = Arena<m_Capacity>();
    }
    ~MemoryManager()
    {
        m_LocalArena.Free();
    }

    template <class T, size_t Num, size_t Align = 0>
    constexpr [[nodiscard]] T* allocate()
    {
        static_assert(Num > 0 && sizeof(T) * Num <= std::numeric_limits<uint32_t>::max(), "Allocation size must be > 0");

        constexpr uint32_t requestedSize = sizeof(T) * Num;
        constexpr uint32_t ceiledSize = get_pow2_ceil<requestedSize>();
        constexpr uint32_t slotsNumber = PAGE_SIZE / ceiledSize;
        constexpr bool isObjectLarge = ceiledSize >= SIZE_OF_LARGE_TYPE;
        if constexpr (!isObjectLarge)
        {
            Pool<ceiledSize, slotsNumber>* pool = nullptr;

            pool = m_LocalArena.GetPool<ceiledSize, slotsNumber>();
            if (!pool)
                pool = m_LocalArena.CreatePool<ceiledSize, slotsNumber>();

            if (pool)
            {
                unsigned char* requestedMemory = pool->RequestSlot();
                if (!requestedMemory)
                {
                    // Not enought space in chunk
                    // Trying to get another chunk for that pool from Arena
                    auto newChunk = m_LocalArena.TryToGetNewChunk<ceiledSize, slotsNumber>(pool);
                    if (newChunk)
                    {
                        pool->Extend(newChunk);
                        return reinterpret_cast<T*>(newChunk->RequestSlotRecursive());
                    }
                    else
                    {
                        // TODO Not enought space in arena for new chunk, allocate new arena?
                        return nullptr;
                    }
                }
                else
                {
                    return reinterpret_cast<T*>(requestedMemory);
                }
            }
            else
            {
                // TODO couldn't create new Pool
            }
        }
        else
        {
            // TODO think about large object strategy
        }
    }

    template <class T, uint32_t num>
    void deallocate(void* ptr)
    {
        constexpr uint32_t deallocationSize = sizeof(T) * num;
        assert(deallocationSize <= std::numeric_limits<uint32_t>::max());

        constexpr uint32_t ceiledSize = get_pow2_ceil<deallocationSize>();
        constexpr uint32_t slotsNumber = PAGE_SIZE / ceiledSize;

        unsigned char* charPtr = reinterpret_cast<unsigned char*>(ptr);
        const bool isObjectLarge = deallocationSize >= SIZE_OF_LARGE_TYPE;
        if (!isObjectLarge)
        {
            auto pool = m_LocalArena.GetPool<ceiledSize, slotsNumber>();
            if (pool)
            {
                pool->GetChunkByPtrToBlock(charPtr)->deallocate(charPtr, num);
            }
        }
    }

    void Print()
    {
        m_LocalArena.Print();
    }

private:
    static constexpr size_t m_Capacity = get_pow2_ceil<Size>();
    Arena<m_Capacity> m_LocalArena;
};

// template <class T>
// class PoolAllocator
//{
// public:
//     using value_type = T;
//
//     PoolAllocator(MemoryManager* mngr)
//         : manager(mngr)
//     {
//     }
//
//     void* allocate(size_t num, size_t align)
//     {
//     }
//
//     void deallocate(size_t num, size_t align)
//     {
//     }
//
// private:
//     MemoryManager* manager;
// };
}  // namespace MM