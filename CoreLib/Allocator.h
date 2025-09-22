#pragma once
#include <chrono>
#include <iostream>
#include <unordered_map>
namespace Fleur::Core
{

static constexpr size_t PAGE_SIZE = 4 * 1024 * 1024;

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
    Benchmark();
    ~Benchmark();

    void Start();
    void End();
    static void Print()
    {
        std::cout << "Number of Allocation: " << m_NumAllocations << std::endl
                  << "Number of deallocations: " << m_NumDeallocations << std::endl
                  << "All allocations time: " << FormatToSecMsMcs(m_SumAllocTime) << std::endl
                  << "longest allocation time: " << FormatToSecMsMcs(m_LongestAllocTime) << std::endl
                  << "Average allocation time: " << FormatToSecMsMcs(m_AverageAllocTime) << std::endl
                  << std::endl;
    }
    void Deallocate();
    static void Frame();
    static void EndOfFrame();
    static std::string FormatToSecMsMcs(std::chrono::microseconds timer);
};

template <typename T>
struct CustomAllocator
{
    using value_type = T;

    constexpr CustomAllocator() noexcept
    {
        mark = Benchmark();
    }
    constexpr ~CustomAllocator() = default;
    template <class U>
    constexpr CustomAllocator(const CustomAllocator<U>&) noexcept
    {
    }
    [[nodiscard]] constexpr T* allocate(size_t n)
    {
        FL_CORE_INFO("[ALLOCATOR] Allocated {0} bytes for {1} of type", n * sizeof(T), n);
        mark.Start();
        T* ptr = static_cast<T*>(malloc(n * sizeof(T)));
        mark.End();
        return ptr;
    }
    constexpr void deallocate(T* p, size_t n)
    {
        FL_CORE_INFO("[ALLOCATOR] Deallocated {0} bytes for {1} of type", n * sizeof(T), n);
        free(p);

        mark.Deallocate();
    }
    bool operator==(const CustomAllocator&) const noexcept
    {
        return true;
    }
    bool operator!=(const CustomAllocator&) const noexcept
    {
        return false;
    }
    Benchmark mark;
};

template <class Ty>
struct Chunk
{
public:
    Chunk(Ty* ptr)
    {
        m_Current = m_Head = ptr;
        m_Tail = m_Head + m_Capacity;
    }

    void SetNextChunk(Chunk<Ty>* nextChunk)
    {
        assert(nextChunk);

        if (!next)
            next = nextChunk;
        else
            next->SetNextChunk(nextChunk);
    }

    template <size_t Num>
    Ty* RequestMemory()
    {
        Ty* testPtr = m_Current + Num;
        if (testPtr <= m_Tail)
        {
            Ty* prevPtr = m_Current;
            m_Current = testPtr;
            m_Used += Num;
            return prevPtr;
        }

        if (next)
        {
            next->RequestMemory<Num>();
        }

        return nullptr;
    }

    void Print()
    {
        std::cout << "\nChunk: " << std::to_string(m_Used) << " - " << m_Capacity << "\n ";

        Ty* prev = m_Head;
        bool inRange = false;

        for (size_t i = 0; i < m_Capacity; i++)
        {
            Ty* ptr = m_Head + i;

            if (i > 0 && *ptr == *prev)
            {
                if (!inRange)
                {
                    std::cout << "[" << i - 1 << ": " << *prev << " - ";
                    inRange = true;
                }
            }
            else
            {
                if (inRange)
                {
                    std::cout << i - 1 << ": " << *prev << "], ";
                    inRange = false;
                }

                std::cout << "[" << i << ": " << *ptr << "], ";
            }

            prev = ptr;
        }

        if (inRange)
        {
            std::cout << m_Capacity - 1 << ": " << *prev << "], ";
        }

        std::cout << std::endl;
    }


private:
    const size_t m_Capacity = PAGE_SIZE / sizeof(Ty);
    size_t m_Used = 0;
    Ty* m_Head = nullptr;
    Ty* m_Tail = nullptr;
    Ty* m_Current = nullptr;
    Chunk<Ty>* next = nullptr;
};

template <class Ty>
struct Pool
{
    Pool(unsigned char* ptr)
    {
        m_HeadChunk = new Chunk<Ty>(reinterpret_cast<Ty*>(ptr));
    }

    template <size_t Num>
    Ty* RequestMemory()
    {
        return m_HeadChunk->RequestMemory<Num>();
    }

    void Extend(Chunk<Ty>* chunk)
    {
        m_HeadChunk->SetNextChunk(chunk);
    }

    void Print()
    {
        std::cout << "\nPrinting Pool: \n";
        m_HeadChunk->Print();
    }

private:
    Chunk<Ty>* m_HeadChunk;
};

template <unsigned int Size>
struct Arena
{
    Arena()
    {
        m_Capacity = Size;
        m_Head = static_cast<unsigned char*>(malloc(m_Capacity));
        memset(m_Head, 0, m_Capacity);

        // assert(m_Head);

        m_Current = m_Head;
        m_Tail = m_Head + (m_Capacity - 1);
    }

    template <class Ty>
    constexpr Pool<Ty>* GetPool()
    {
        uint32_t power = static_cast<uint32_t>(std::ceil(std::log2(sizeof(Ty))));

        if (IsPoolCreated(power))
            return static_cast<Pool<Ty>*>(map[power]);

        // Test if we have enought capacity
        unsigned char* testPtr = m_Current += PAGE_SIZE;
        if (testPtr <= m_Tail)
        {
            map[power] = new Pool<Ty>(m_Current);
            m_Current += PAGE_SIZE;
            BitSet(power);
            return static_cast<Pool<Ty>*>(map[power]);
        }
        else
        {
            // Not enought space in current arena
        }
    }

    template <class Ty>
    Chunk<Ty>* TryToGetNewChunk(Pool<Ty>* pool)
    {
        if (m_Current + PAGE_SIZE <= m_Tail)
        {
            // Arena has enought space for new chunk, give it
            Ty* ptr = reinterpret_cast<Ty*>(m_Current);
            m_Current += PAGE_SIZE;
            return new Chunk<Ty>(ptr);
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

private:
    unsigned char* m_Head;
    unsigned char* m_Tail;
    unsigned char* m_Current;
    size_t m_Capacity;
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

    template <class T, size_t Num, size_t Align = 0>
    constexpr [[nodiscard]] T* allocate()
    {
        Pool<T>* pool = m_LocalArena.GetPool<T>();
        T* ptrType = pool->RequestMemory<static_cast<size_t>(Num)>();
        if (!ptrType)
        {
            // Not enought space in chunk
            // Trying to get another chunk for that pool from Arena
            Chunk<T>* newChunk = m_LocalArena.TryToGetNewChunk<T>(pool);
            if (newChunk)
            {
                pool->Extend(newChunk);
                return newChunk->RequestMemory<Num>();
            }
            else
            {
                // TODO Not enought space in arena, allocate new arena?
                return nullptr;
            }
        }
    }
    void Print()
    {
        auto Pool = m_LocalArena.GetPool<int>();
        Pool->Print();
    }

private:
    static constexpr uint32_t m_Power = ceil_log2(Size);
    static constexpr size_t m_Capacity = get_powered_size(m_Power);
    Arena<m_Capacity> m_LocalArena;
};


}  // namespace Fleur::Core