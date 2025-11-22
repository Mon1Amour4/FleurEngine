#include "MemoryManager.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//======================================================================
// Benchmark
MM::Benchmark::Benchmark(float AvgPeriodSecs)
    : m_AveragePeriod(AvgPeriodSecs)
    , m_FramesAverage(0)
    , m_AverageTimer(0)
    , m_FramesPerSecondTimer(0)
    , m_NumAllocations(0)
    , m_NumDeallocations(0)
    , m_LongestAllocTime(0)
    , m_LongestDeallocTime(0)
    , m_AverageAllocTime(0)
    , m_AverageDeallocTime(0)
    , m_OverallAllocTime(0)
    , m_OverallDeallocTime(0)
    , m_OverallTime(0)
    , m_FrameAllocTime(0)
    , m_FrameDeallocTime(0)
    , m_FramesPerSecond(0)
    , m_StartAllocTimer(std::chrono::time_point<std::chrono::steady_clock>(std::chrono::steady_clock::duration::zero()))
    , m_StartDeallocTimer(std::chrono::time_point<std::chrono::steady_clock>(std::chrono::steady_clock::duration::zero()))
    , m_AveragePeriodTime(0)
{
    assert(m_AveragePeriod > 0);
}
MM::Benchmark::~Benchmark()
{
    EndAlloc();
    EndDealloc();
}

void MM::Benchmark::StartAlloc()
{
    ++m_NumAllocations;
    m_StartAllocTimer = std::chrono::steady_clock::now();
}
void MM::Benchmark::StartDealloc()
{
    ++m_NumDeallocations;
    m_StartDeallocTimer = std::chrono::steady_clock::now();
}

void MM::Benchmark::EndAlloc()
{
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_StartAllocTimer);
    if (m_StartAllocTimer.time_since_epoch().count() == 0)
        return;

    m_OverallAllocTime += duration;
    if (duration > m_LongestAllocTime)
        m_LongestAllocTime = duration;
    m_FrameAllocTime += duration;
    m_OverallTime += duration;

    m_StartAllocTimer = std::chrono::time_point<std::chrono::steady_clock>(std::chrono::steady_clock::duration::zero());
}
void MM::Benchmark::EndDealloc()
{
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_StartDeallocTimer);
    if (m_StartDeallocTimer.time_since_epoch().count() == 0)
        return;

    m_OverallDeallocTime += duration;
    if (duration > m_LongestDeallocTime)
        m_LongestDeallocTime = duration;
    m_FrameDeallocTime += duration;

    m_OverallTime += duration;
    m_StartDeallocTimer = std::chrono::time_point<std::chrono::steady_clock>(std::chrono::steady_clock::duration::zero());
}

void MM::Benchmark::Print()
{
    uint64_t bufferSize = 4096;
    char* buffer = new char[bufferSize];
    buffer[bufferSize - 1] = '\0';
    char* tmp = buffer;
    int written = 0;
    // clang-format off
    written += sprintf_s(tmp + written, bufferSize - written, "%s", "\n\n\n");
    written += sprintf_s(tmp + written, bufferSize - written,
        "////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n"
        "//                                                                                                                                                //\n"
        "//                                                         MEMORY BENCHMARK REPORT                                                                //\n"
        "//                                                                                                                                                //\n"
        "////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n");

    written += sprintf_s(tmp + written, bufferSize - written, 
        "|%-23s|%-37s|%-26s|%-26s|%-35s|\n", 
        "Number of Allocation", 
        "Average allocation time per second", 
        "Longest allocation time",
        "Overall Allocation time", "Overall Time");

    written += sprintf_s(tmp + written, bufferSize - written, 
        "|%-23d|%-37s|%-26s|%-26s|%-35s|\n", 
        static_cast<int>(m_NumAllocations), 
        FormatToSeconds(m_AverageAllocTime).c_str(),
        FormatToSeconds(m_LongestAllocTime).c_str(), 
        FormatToSeconds(m_OverallAllocTime).c_str(), 
        FormatToSeconds(m_OverallTime).c_str());

    written += sprintf_s(tmp + written, bufferSize - written, 
        "|%-23s|%-37s|%-26s|%-26s|%-35s|\n", 
        "Number of Deallocation", 
        "Average deallocation time per second", 
        "Longest deallocation time",
        "Overall Deallocation time","");

    written += sprintf_s(tmp + written, bufferSize - written, 
        "|%-23d|%-37s|%-26s|%-26s|%-35s|\n", 
        static_cast<int>(m_NumDeallocations), 
        FormatToSeconds(m_AverageDeallocTime).c_str(),
        FormatToSeconds(m_LongestDeallocTime).c_str(), 
        FormatToSeconds(m_OverallDeallocTime).c_str(), "");

    written += sprintf_s(tmp + written, bufferSize - written, 
        "////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n"
        "//                                                                                                                                                //\n"
        "//                                                      END OF MEMORY BENCHMARK REPORT                                                            //\n"
        "//                                                                                                                                                //\n"
        "////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n");
    written += sprintf_s(tmp + written, bufferSize - written, "%s", "\n\n\n");
    // clang-format on
    std::cout << buffer;

    delete[] buffer;
}

void MM::Benchmark::Tick(float dtTime)
{
    m_FramesPerSecondTimer += dtTime;
    m_AverageTimer += dtTime;

    ++m_FramesPerSecond;
    ++m_FramesAverage;

    if (m_FramesPerSecondTimer >= 1.0f)
    {
        if (m_FramesPerSecond > 0)
            m_AverageAllocTime = m_FrameAllocTime / m_FramesPerSecond;

        m_FrameAllocTime = std::chrono::microseconds(0);
        m_FramesPerSecond = 0;
        m_FramesPerSecondTimer = 0.0f;
    }

    if (m_AverageTimer >= m_AveragePeriod)
    {
        if (m_FramesAverage > 0)
            m_AveragePeriodTime = m_AverageTimer / m_FramesAverage;
        else
            m_AveragePeriodTime = 0.0f;

        m_AverageTimer = 0.0f;
        m_FramesAverage = 0;
    }
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
std::string MM::Benchmark::FormatToSeconds(std::chrono::microseconds timer)
{
    double seconds = std::chrono::duration<double>(timer).count();

    std::string str = std::to_string(seconds);
    std::string another;

    auto pos = str.find('.');
    str.insert(pos + 4, 1, '\'');

    str += "s";
    return str;
}


//======================================================================
// Memory Manager
MM::MemoryManager::MemoryManager(unsigned char* memoryStart, size_t offset, size_t capacity)
    : m_PageSize(PAGE_SIZE)
    , m_MinSlotSize(16)
    , m_Capacity(capacity)
    , m_Head(memoryStart)
    , m_UsedBytes(0)
    , slubAlloc(nullptr)
    , pageAlloc(nullptr)
{
    MM_ASSERT(m_Capacity > 0 && m_PageSize > 0);

    MM_PRINT("Memory Manager has allocated " << std::to_string(capacity) << " bytes\n");

    // PageAllocator:
    uint32_t pageAllocOffset = offset + sizeof(SLUBAllocator);
    uint32_t pageAllocContent = pageAllocOffset + sizeof(PageAllocator);
    pageAlloc = new (m_Head + pageAllocOffset) PageAllocator(memoryStart + 4096, m_Capacity, m_PageSize);

    MM_ASSERT(pageAlloc);

    // SLUBAllocator:
    slubAlloc = new (m_Head + offset) SLUBAllocator(pageAlloc, m_PageSize, m_MinSlotSize);

    MM_ASSERT(slubAlloc);
    MM_DEBUG_BREAK(TOCHARPTR(slubAlloc) - memoryStart != sizeof(MM::MemoryManager));
    MM_DEBUG_BREAK(TOCHARPTR(pageAlloc) - memoryStart != (sizeof(MM::MemoryManager) + sizeof(SLUBAllocator)));
}
MM::MemoryManager::~MemoryManager()
{
    // TODO
}

MM::MemoryManager* MM::MemoryManager::ManagerFabric(size_t capacity)
{
    MM_ASSERT(capacity > 0);

    size_t alignedCapacity = capacity / 4096 * 4096;

    void* rawPtr = VirtualAlloc(NULL, alignedCapacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    MM_ASSERT(rawPtr);

    unsigned char* charPtr = reinterpret_cast<unsigned char*>(rawPtr);
    return new (rawPtr) MM::MemoryManager(charPtr, sizeof(MM::MemoryManager), capacity);
}


void MM::MemoryManager::Print()
{
    // TODO;
}
std::string MM::MemoryManager::GetSnapshot() const
{
    // TODO: Fix
    /*std::string str;

    uint64_t bufferSize = 1024l * 1024l * 10;
    char* buffer = new char[bufferSize];
    buffer[bufferSize - 1] = '\0';
    char* tmp = buffer;

    ArenaSnapshot(tmp);

    str = buffer;
    delete[] buffer;
    return str;*/
    return std::string();
}

uint32_t MM::MemoryManager::CalculateSlotSize(uint32_t original)
{
    uint32_t slotSize = get_pow2_ceil(original);
    if (slotSize < m_MinSlotSize)
        slotSize = m_MinSlotSize;
    return slotSize;
}

//======================================================================
// Arena
// void MM::Arena::Print()
//{
//    std::cout << "//---------------------------- ARENA-PRINTING ----------------------------\\\n";
//
//    float percentage = (m_UsedBytes / (float)m_CapacityBytes) * 100;
//    std::cout << "Arena: " << m_UsedBytes << "/" << m_CapacityBytes << " - " << percentage << "%";
//    uint32_t offset = m_PageSize + sizeof(Pool) + sizeof(Chunk);
//    for (size_t i = 0; m_MinSlotSize << i <= m_PageSize; i++)
//    {
//        uint32_t size = m_MinSlotSize << i;
//        reinterpret_cast<Pool*>(m_Head + offset * i)->Print();
//    }
//
//    std::cout << "\n//--------------------------------- END ----------------------------\\ \n";
//}
// void MM::Arena::ArenaSnapshotToStream(std::ofstream& stream)
//{
//    stream << "\n//---------------------------- ARENA-PRINTING ----------------------------\\ \n";
//    float percentage = (m_UsedBytes / (float)m_CapacityBytes) * 100;
//    stream << "Arena: " << m_UsedBytes << "/" << m_CapacityBytes << " - " << percentage << "%";
//
//    uint32_t offset = m_PageSize + sizeof(Pool) + sizeof(Chunk);
//    for (size_t i = 0; m_MinSlotSize << i <= m_PageSize; i++)
//    {
//        uint32_t size = m_MinSlotSize << i;
//        reinterpret_cast<Pool*>(m_Head + offset * i)->PoolSpapshotToStream(stream);
//    }
//
//    stream << "\n//--------------------------------- END ----------------------------\\ \n";
//}
// void MM::Arena::ArenaSnapshot(char* buffer)
//{
//    buffer += std::sprintf(buffer, "//---------------------------- ARENA-PRINTING ----------------------------\\\n");
//    float percentage = (m_UsedBytes / (float)m_CapacityBytes) * 100;
//    buffer += std::sprintf(buffer, "Arena: %zu/%zu - %f%%\n", m_UsedBytes, m_CapacityBytes, static_cast<float>(percentage));
//
//    uint32_t poolsCount = CountSlots(m_PageSize, 16) - 1;
//    uint32_t offset = sizeof(Pool);
//
//    // Arena Free list:
//    buffer += std::sprintf(buffer, " Free Chunks:\n");
//    void* chunk = m_ChunkCache;
//    uint32_t counter = 0;
//    do
//    {
//        buffer += std::sprintf(buffer, " Free Chunk_%d {0x%p}\n", counter, chunk);
//        if (chunk)
//        {
//            chunk = *reinterpret_cast<void**>(chunk);
//            ++counter;
//        }
//    } while (chunk);
//
//    for (size_t i = 0; i < poolsCount; i++)
//    {
//        uint32_t poolSize = m_MinSlotSize + (8 * i);
//        Pool* pool = reinterpret_cast<Pool*>(m_Head + offset * i);
//        pool->PoolSpapshot(buffer);
//    }
//    buffer += std::sprintf(buffer, "//--------------------------------- END ----------------------------\\ \n");
//}


//======================================================================
// MemoryInfo
void MemoryInfo::AddAlloc(size_t slotSize)
{
    if (auto info = infos.find(slotSize); info != infos.end())
    {
        info->second.allocAmount++;
        info->second.overallMemoryBytes += slotSize;
        info->second.size = slotSize;
    }
    else
    {
        infos.insert({slotSize, record(slotSize)});
    }
}
void MemoryInfo::AddDealloc(size_t slotSize)
{
    auto info = infos.find(slotSize);
    info->second.deallocAmount++;
    info->second.size = slotSize;
}

inline std::string MemoryInfo::FormatBytes(size_t bytes)
{
    {
        size_t GB = bytes / 1024 / 1024 / 1024;
        bytes -= GB * 1024 * 1024 * 1024;

        size_t MB = bytes / 1024 / 1024;
        bytes -= MB * 1024 * 1024;

        size_t KB = bytes / 1024;
        bytes -= KB * 1024;

        std::string str;
        if (GB > 0)
            str += std::to_string(GB) + "'GB ";
        if (MB > 0)
            str += std::to_string(MB) + "'MB ";
        if (KB > 0)
            str += std::to_string(KB) + "'KB ";

        str += std::to_string(bytes) + "'B ";

        return str;
    }
}
void MemoryInfo::Print() const
{
    std::vector<std::pair<uint32_t, record>> items(infos.begin(), infos.end());
    std::sort(items.begin(), items.end());

    size_t size = 1024 * 1024;
    char* buffer = new char[size];
    buffer[size - 1] = '\0';

    char* tmp = buffer;

    tmp += sprintf(tmp, "%-4s|%-11s|%-13s|%-18s|\n", "Size", "AllocAmount", "DeallocAmount", "OverallMemoryBytes");
    for (const auto& info : items)
    {
        tmp += sprintf(tmp, "%-4d|%-11d|%-13d|%-18s|\n", info.second.size, info.second.allocAmount, info.second.deallocAmount,
                       MemoryInfo::FormatBytes(info.second.overallMemoryBytes).c_str());
    }
    std::cout << std::endl << buffer << std::endl;

    delete[] buffer;
}