#pragma once
#include <chrono>
#include <iostream>
namespace Fleur::Core
{

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
}  // namespace Fleur::Core