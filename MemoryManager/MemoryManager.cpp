#include "MemoryManager.h"

// Benchmark
size_t MM::Benchmark::m_NumAllocations = 0;
size_t MM::Benchmark::m_NumDeallocations = 0;
std::chrono::microseconds MM::Benchmark::m_LongestAllocTime{0};
std::chrono::microseconds MM::Benchmark::m_AverageAllocTime{0};
std::chrono::microseconds MM::Benchmark::m_SumAllocTime{0};
std::chrono::microseconds MM::Benchmark::m_FrameAllocTime{0};
size_t MM::Benchmark::frames = 0;

void MM::Benchmark::Start()
{
    ++m_NumAllocations;
    start = std::chrono::steady_clock::now();
}

void MM::Benchmark::End()
{
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    m_SumAllocTime += duration;
    if (duration > m_LongestAllocTime)
        m_LongestAllocTime = duration;
    m_FrameAllocTime += duration;
}

void MM::Benchmark::Print()
{
    std::cout << "Number of Allocation: " << m_NumAllocations << std::endl
              << "Number of deallocations: " << m_NumDeallocations << std::endl
              << "All allocations time: " << FormatToSecMsMcs(m_SumAllocTime) << std::endl
              << "longest allocation time: " << FormatToSecMsMcs(m_LongestAllocTime) << std::endl
              << "Average allocation time: " << FormatToSecMsMcs(m_AverageAllocTime) << std::endl
              << std::endl;
}

void MM::Benchmark::EndOfFrame()
{
    m_AverageAllocTime = m_FrameAllocTime / frames;
    frames = 0;
    m_FrameAllocTime = std::chrono::microseconds(0);
}

std::string MM::Benchmark::FormatToSecMsMcs(std::chrono::microseconds timer)
{
    std::string str;
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(timer);
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(timer - secs);
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(timer - secs - millis);
    str += std::to_string(secs.count());
    str += "s ";
    str += std::to_string(millis.count());
    str += "ms ";
    str += std::to_string(micros.count());
    str += "mcs";
    return str;
}


// FreeBlock

// MM::FreeBlock::FreeBlock(unsigned char* ptr, uint32_t size)
//     : m_Ptr(ptr)
//     , m_Size(size)
//     , m_Next(nullptr)
//     , m_Prev(nullptr)
//{
// }
//
// auto MM::FreeBlock::operator<=>(const FreeBlock& other) const
//{
//     std::uintptr_t thisPtr = reinterpret_cast<std::uintptr_t>(m_Ptr);
//     std::uintptr_t otherPtr = reinterpret_cast<std::uintptr_t>(other.m_Ptr);
//     return thisPtr <=> otherPtr;
// }
//
// void MM::FreeBlock::RemoveBlock()
//{
//     if (m_Prev)
//     {
//         if (m_Next)
//             m_Prev->m_Next = m_Next;
//         else
//             m_Prev->m_Next = nullptr;
//     }
// }
//
// void MM::FreeBlock::SetNext(FreeBlock* next)
//{
//     if (!m_Next)
//         m_Next = next;
//     else
//         m_Next->SetNext(next);
// }
