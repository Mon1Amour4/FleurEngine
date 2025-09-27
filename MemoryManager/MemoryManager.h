#pragma once

namespace MM
{
static constexpr size_t PAGE_SIZE = 4 * 1024 * 1024;
static constexpr size_t SIZE_OF_LARGE_TYPE = 1024 * 1;

consteval size_t get_powered_size(size_t power)
{
    return size_t{1} << power;
}
constexpr uint32_t ceil_log2(uint32_t n)
{
    uint32_t p = 0;
    uint32_t v = 1;
    while (v < n)
    {
        v <<= 1;
        p++;
    }
    return p;
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

class FreeBlock
{
public:
    FreeBlock(unsigned char* ptr, uint32_t size);
    ~FreeBlock() = default;

    auto operator<=>(const FreeBlock& other) const;

    void RemoveBlock();

    void SetNext(FreeBlock* next);

private:
    unsigned char* m_Ptr;
    uint32_t m_Size;
    FreeBlock* m_Next;
    FreeBlock* m_Prev;
};

template <size_t TypeSize, size_t NumObjects>
struct Chunk
{
public:
    Chunk(unsigned char* ptr)
    {
        // Chunk size must not exceed uint32_t size
        assert(m_CapacityBytes <= UINT32_MAX);

        m_Current = m_Head = ptr;
        m_Tail = m_Head + m_CapacityBytes;
    }

    void SetNextChunkRecursive(Chunk<TypeSize, NumObjects>* nextChunk)
    {
        assert(nextChunk);

        if (!next)
            next = nextChunk;
        else
            next->SetNextChunkRecursive(nextChunk);
    }

    template <uint32_t Num>
    unsigned char* RequestMemory()
    {
        constexpr uint32_t requestedSize = Num * TypeSize;
        unsigned char* requestedPtr = m_Current + (requestedSize - 1);
        if (requestedPtr < m_Tail)
        {
            unsigned char* prevPtr = m_Current;
            m_Current = requestedPtr + 1;
            m_UsedBytes += requestedSize;
            return prevPtr;
        }

        if (next)
        {
            next->RequestMemory<NumObjects>();
        }

        return nullptr;
    }

    bool deallocate(unsigned char* ptr, uint32_t num)
    {
        uint32_t requestedSize = TypeSize * num;
        unsigned char* blockEndPtr = ptr + requestedSize;
        if (blockEndPtr < m_Current)
        {
            m_UsedBytes -= requestedSize;
            FreeBlock* block = new FreeBlock(ptr, requestedSize);
            if (!m_HeadFreeBlock)
            {
                m_HeadFreeBlock = block;
                return true;
            }
            else
            {
                m_HeadFreeBlock->SetNext(block);
                return true;
            }
        }
        else if (blockEndPtr == m_Current)
        {
            unsigned char* deallocatedPtr = m_Current - (TypeSize * num);
            if (deallocatedPtr >= m_Head)
            {
                m_Current = deallocatedPtr;
                m_UsedBytes -= requestedSize;
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

        const uint32_t numCells = m_CapacityBytes / TypeSize;

        std::cout << "\nBucket View (" << m_UsedBytes << "/" << m_CapacityBytes << ")\n";

        uint32_t filled = m_UsedBytes / TypeSize;
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

    void Print()
    {
        static constexpr char emptyCell = ' ';
        static constexpr char occupiedCell = 'x';
        static constexpr uint32_t cellsPerRow = 40;

        std::cout << "\nChunk: " << m_UsedBytes << " / " << m_CapacityBytes << "\n";

        const uint32_t numCells = m_CapacityBytes / TypeSize;

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
                unsigned char* ptr = m_Head + (i * TypeSize);
                std::cout << "[ " << (ptr < m_Current ? occupiedCell : emptyCell) << "] ";
            }
            std::cout << "\n\n";
        }
        PrintBucket();

        if (next)
            next->Print();
    }

    unsigned char* Tail() const
    {
        return m_Tail;
    }

    Chunk<TypeSize, NumObjects>* IsPtrToBlockIsInChunkRecursive(unsigned char* ptr)
    {
        if (ptr < m_Tail && ptr >= m_Head)
            return this;

        if (next)
            return next->IsPtrToBlockIsInChunkRecursive(ptr);
        else
            return nullptr;
    }

private:
    const uint32_t m_CapacityBytes = NumObjects * TypeSize;
    uint32_t m_UsedBytes = 0;
    unsigned char* m_Head = nullptr;
    unsigned char* m_Tail = nullptr;
    unsigned char* m_Current = nullptr;
    Chunk<TypeSize, NumObjects>* next = nullptr;
    FreeBlock* m_HeadFreeBlock = nullptr;
};

template <size_t TypeSize, size_t NumObjects>
struct Pool
{
    Pool(unsigned char* ptr)
    {
        m_HeadChunk = new Chunk<TypeSize, NumObjects>(ptr);
        ++m_NumChunks;
    }

    template <size_t Num>
    unsigned char* RequestMemory()
    {
        return m_HeadChunk->RequestMemory<Num>();
    }

    void Extend(Chunk<TypeSize, NumObjects>* chunk)
    {
        m_HeadChunk->SetNextChunkRecursive(chunk);
        m_NumChunks++;
    }

    void Print()
    {
        std::cout << "\nPrinting Pool: \n" << "Number of chunks: " << std::to_string(m_NumChunks) << "\n";
        m_HeadChunk->Print();
    }

    Chunk<TypeSize, NumObjects>* GetChunkByPtrToBlock(unsigned char* ptr)
    {
        if (m_HeadChunk)
            return m_HeadChunk->IsPtrToBlockIsInChunkRecursive(ptr);
        else
            return nullptr;
    }

private:
    Chunk<TypeSize, NumObjects>* m_HeadChunk;
    uint32_t m_NumChunks = 0;
};

template <unsigned int Size>
struct Arena
{
    Arena()
    {
        m_CapacityBytes = Size;
        m_Head = static_cast<unsigned char*>(malloc(m_CapacityBytes));
        memset(m_Head, 0, m_CapacityBytes);

        // assert(m_Head);

        m_Current = m_Head;
        m_Tail = m_Head + (m_CapacityBytes);
    }

    template <uint32_t TypeSize, size_t NumObjects = PAGE_SIZE / TypeSize>
    [[nodiscard]] constexpr Pool<TypeSize, NumObjects>* GetPool()
    {
        constexpr size_t requestedSize = TypeSize * NumObjects;
        uint32_t power = static_cast<uint32_t>(std::ceil(std::log2(TypeSize)));

        if (IsPoolCreated(power))
            return static_cast<Pool<TypeSize, NumObjects>*>(map[power]);

        // Test if we have enought capacity
        unsigned char* requestedPtr = m_Current + requestedSize;
        if (requestedPtr < m_Tail)
        {
            map[power] = new Pool<TypeSize, NumObjects>(m_Current);
            m_Current = requestedPtr;
            m_UsedBytes += requestedSize;
            BitSet(power);
            return static_cast<Pool<TypeSize, NumObjects>*>(map[power]);
        }
        else
        {
            // Not enought space in current arena
            return nullptr;
        }
    }

    template <size_t TypeSize, size_t NumObjects = PAGE_SIZE / TypeSize>
    [[nodiscard]] constexpr Chunk<TypeSize, NumObjects>* TryToGetNewChunk(Pool<TypeSize, NumObjects>* pool)
    {
        unsigned char* requestedPtr = m_Current + NumObjects * TypeSize;
        if (requestedPtr < m_Tail)
        {
            // Arena has enought space for new chunk, give it
            unsigned char* prevPtr = m_Current;
            m_Current = requestedPtr;
            return new Chunk<TypeSize, NumObjects>(prevPtr);
        }
        else
        {
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

private:
    unsigned char* m_Head;

    // one-past-the-end
    unsigned char* m_Tail;

    unsigned char* m_Current;
    size_t m_CapacityBytes, m_UsedBytes = 0;
    uint32_t bitmap{0};

    std::unordered_map<uint32_t, void*> map;

    bool IsPoolCreated(uint32_t power)
    {
        return BitScan(power);
    }

    bool BitScan(uint32_t num)
    {
        uint32_t mask = 1 << num;
        return (bitmap & mask);
    }
    void BitSet(uint32_t num)
    {
        uint32_t mask = 1 << num;
        bitmap |= mask;
    }
};

template <unsigned int Size>
struct MemoryManager
{
    MemoryManager()
    {
        m_LocalArena = Arena<m_Capacity>();
    }
    ~MemoryManager()
    {
        m_LocalArena.Free();
    }

    template <class T, size_t Num, size_t Align = 0>
    constexpr [[nodiscard]] T* allocate()
    {
        constexpr size_t typeSize = sizeof(T);
        constexpr bool isObjectLarge = typeSize >= SIZE_OF_LARGE_TYPE;
        if (!isObjectLarge)
        {
            auto pool = m_LocalArena.GetPool<typeSize, 80>();
            if (pool)
            {
                unsigned char* requestedMemory = pool->RequestMemory<Num>();
                if (!requestedMemory)
                {
                    // Not enought space in chunk
                    // Trying to get another chunk for that pool from Arena
                    auto newChunk = m_LocalArena.TryToGetNewChunk<typeSize>(pool);
                    if (newChunk)
                    {
                        pool->Extend(newChunk);
                        return reinterpret_cast<T*>(newChunk->RequestMemory<Num>());
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
                // TODO there is no Pool for this type size AND no space for new Pool in current Arena
            }
        }
        else
        {
            // TODO think about large object strategy
        }
    }

    template <class T>
    void deallocate(void* ptr, size_t num)
    {
        unsigned char* charPtr = reinterpret_cast<unsigned char*>(ptr);
        constexpr size_t typeSize = sizeof(T);
        constexpr bool isObjectLarge = typeSize >= SIZE_OF_LARGE_TYPE;
        if (!isObjectLarge)
        {
            auto pool = m_LocalArena.GetPool<typeSize, 80>();
            if (pool)
            {
                pool->GetChunkByPtrToBlock(charPtr)->deallocate(charPtr, num);
            }
        }
    }

    void Print()
    {
        auto Pool = m_LocalArena.GetPool<sizeof(int)>();
        Pool->Print();
    }

private:
    static constexpr uint32_t m_Power = ceil_log2(Size);
    static constexpr size_t m_Capacity = get_powered_size(m_Power);
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