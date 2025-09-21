#include "Allocator.h"

size_t Fleur::Core::Benchmark::m_NumAllocations = 0;
size_t Fleur::Core::Benchmark::m_NumDeallocations = 0;
std::chrono::microseconds Fleur::Core::Benchmark::m_LongestAllocTime{0};
std::chrono::microseconds Fleur::Core::Benchmark::m_AverageAllocTime{0};
std::chrono::microseconds Fleur::Core::Benchmark::m_SumAllocTime{0};
std::chrono::microseconds Fleur::Core::Benchmark::m_FrameAllocTime{0};
size_t Fleur::Core::Benchmark::frames = 0;

Fleur::Core::Benchmark::Benchmark()
{
}

Fleur::Core::Benchmark::~Benchmark()
{
}
void Fleur::Core::Benchmark::Start()
{
    ++m_NumAllocations;
    start = std::chrono::steady_clock::now();
}

void Fleur::Core::Benchmark::End()
{
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    m_SumAllocTime += duration;
    if (duration > m_LongestAllocTime)
        m_LongestAllocTime = duration;
    m_FrameAllocTime += duration;
}

void Fleur::Core::Benchmark::Deallocate()
{
    ++m_NumDeallocations;
}
void Fleur::Core::Benchmark::Frame()
{
    ++frames;
}

void Fleur::Core::Benchmark::EndOfFrame()
{
    m_AverageAllocTime = m_FrameAllocTime / frames;
    frames = 0;
    m_FrameAllocTime = std::chrono::microseconds(0);
}

std::string Fleur::Core::Benchmark::FormatToSecMsMcs(std::chrono::microseconds timer)
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
