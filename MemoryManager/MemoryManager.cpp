#include "MemoryManager.h"

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
    // clang-format off
    tmp += sprintf_s(tmp, bufferSize, "%s", "\n\n\n");
    tmp += sprintf_s(tmp, bufferSize,
                             "////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n"
                             "//                                                                                                                                                //\n"
                             "//                                                         MEMORY BENCHMARK REPORT                                                                //\n"
                             "//                                                                                                                                                //\n"
                             "////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n");

    tmp += sprintf_s(tmp, bufferSize, "|%-23s|%-37s|%-26s|%-26s|%-35s|\n", "Number of Allocation", "Average allocation time per second", "Longest allocation time",
                        "Overall Allocation time", "Overall Time");

    tmp += sprintf_s(tmp, bufferSize, "|%-23d|%-37s|%-26s|%-26s|%-35s|\n", static_cast<int>(m_NumAllocations), FormatToSeconds(m_AverageAllocTime).c_str(),
                        FormatToSeconds(m_LongestAllocTime).c_str(), FormatToSeconds(m_OverallAllocTime).c_str(), FormatToSeconds(m_OverallTime).c_str());

    tmp += sprintf_s(tmp, bufferSize, "|%-23s|%-37s|%-26s|%-26s|%-35s|\n", "Number of Deallocation", "Average deallocation time per second", "Longest deallocation time",
                        "Overall Deallocation time","");

    tmp += sprintf_s(tmp, bufferSize, "|%-23d|%-37s|%-26s|%-26s|%-35s|\n", static_cast<int>(m_NumDeallocations), FormatToSeconds(m_AverageDeallocTime).c_str(),
                        FormatToSeconds(m_LongestDeallocTime).c_str(), FormatToSeconds(m_OverallDeallocTime).c_str(), "");

    tmp += sprintf_s(tmp, bufferSize, 
                             "////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n"
                             "//                                                                                                                                                //\n"
                             "//                                                      END OF MEMORY BENCHMARK REPORT                                                            //\n"
                             "//                                                                                                                                                //\n"
                             "////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n");
        tmp += sprintf_s(tmp, bufferSize, "%s", "\n\n\n");
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
        else
            m_AverageAllocTime = std::chrono::microseconds(0);

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
    str += "s";
    return str;
}

//======================================================================
// Memory Manager
MM::MemoryManager::MemoryManager(size_t capacity, uint32_t arenaSize, uint32_t pageSize, uint8_t minSlotSize)
    : m_PageSize(pageSize)
    , m_MinSlotSize(minSlotSize)
    , m_ArenaSize(arenaSize)
    , m_Capacity(capacity)
    , m_Head(nullptr)
    , m_UsedBytes(0)
{
    MM_ASSERT(capacity > 0 && pageSize > 0 && minSlotSize > 0);
    MM_ASSERT(arenaSize <= capacity);

    m_Head = reinterpret_cast<unsigned char*>(malloc(capacity));

    MM_ASSERT(m_Head);

    MM_PRINT("Memory Manager has allocated " << std::to_string(capacity) << "bytes\n");

    m_LocalArena = new (static_cast<void*>(m_Head)) Arena(m_Head + sizeof(Arena), arenaSize - sizeof(Arena), pageSize, minSlotSize);

    MM_ASSERT(m_LocalArena);

    m_UsedBytes += m_ArenaSize;
}
MM::MemoryManager::~MemoryManager()
{
    m_LocalArena->~Arena();
}

void MM::MemoryManager::Print()
{
    m_LocalArena->Print();
}
void MM::MemoryManager::SaveSnapshotToFile(std::string_view fileName, uint32_t iteration)
{
    uint32_t bufferSize = 16000;
    char* buffer = new char[bufferSize];
    buffer[bufferSize - 1] = '\0';
    char* tmp = buffer;
    tmp += sprintf_s(buffer, bufferSize, "Iteration: %d\n", iteration);

    m_LocalArena->ArenaSnapshot(tmp);

    std::ofstream myFile;
    myFile.open(fileName.data(), std::ios::app);

    if (myFile.good())
    {
        myFile << buffer;
    }
    myFile.close();


    delete[] buffer;
}
void MM::MemoryManager::ClearFile(std::string_view fileName)
{
    std::ofstream myFile;

    myFile.open(fileName.data());
    myFile.close();
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
MM::Arena::Arena(unsigned char* ptr, size_t capacity, uint32_t pageSize, uint32_t minSlotSize)
    : m_PageSize(pageSize)
    , m_MinSlotSize(minSlotSize)
    , m_CapacityBytes(capacity)
    , m_UsedBytes(0)
    , m_Head(ptr)
{
    MM_ASSERT(m_Head);

    m_Current = m_Head;
    m_Tail = m_Head + (m_CapacityBytes);
}
MM::Arena MM::Arena::operator=(const Arena& other)
{
    return Arena(nullptr, other.m_CapacityBytes, other.m_PageSize, other.m_MinSlotSize);
};

MM::Pool* MM::Arena::CreatePool(uint32_t slotSize)
{
    // Test if we have enought capacity
    uint32_t requestedSize = m_PageSize + sizeof(Pool) + sizeof(Chunk);
    if (m_UsedBytes + requestedSize <= m_CapacityBytes)
    {
        map[slotSize] = new (m_Current) Pool(m_Current + sizeof(Pool), slotSize, static_cast<uint8_t>(m_PageSize / slotSize));
        m_Current += requestedSize;
        m_UsedBytes += requestedSize;
        MM_ASSERT(m_Current < m_Tail);
        return static_cast<Pool*>(map[slotSize]);
    }
    else
    {
        // Not enought space in current arena
        return nullptr;
    }
}
MM::Pool* MM::Arena::GetPool(uint32_t slotSize)
{
    if (auto val = map.find(slotSize); val != map.end())
        return static_cast<Pool*>(val->second);

    return nullptr;
}

MM::Chunk* MM::Arena::TryToGetNewChunk(MM::Pool* pool, uint32_t slotSize, uint8_t slotsCount)
{
    unsigned char* requestedPtr = m_Current + (m_PageSize + sizeof(Chunk));
    if (requestedPtr < m_Tail)
    {
        // Arena has enought space for new chunk, give it
        unsigned char* prevPtr = m_Current;
        m_Current = requestedPtr;

        MM_DEBUG_EXPRESSION({
            Chunk* newChunk = new (prevPtr) Chunk(prevPtr + sizeof(Chunk), slotSize, slotsCount);
            unsigned char* newChunkChar = reinterpret_cast<unsigned char*>(newChunk);

            MM_DEBUG_BREAK(pool->IsInChunkChain(newChunkChar));
            MM_DEBUG_BREAK(m_Current >= m_Tail);
            return newChunk;
        })

        return new (prevPtr) Chunk(prevPtr + sizeof(Chunk), slotSize, slotsCount);
    }
    else
    {
        MM_DEBUG_BREAK(true);
        return nullptr;
    }
}

void MM::Arena::Print()
{
    std::cout << "//---------------------------- ARENA-PRINTING ----------------------------\\\n";

    float percentage = (m_UsedBytes / (float)m_CapacityBytes) * 100;
    std::cout << "Arena: " << m_UsedBytes << "/" << m_CapacityBytes << " - " << percentage << "%";
    for (auto& pair : map)
    {
        reinterpret_cast<Pool*>(pair.second)->Print();
    }

    std::cout << "\n//--------------------------------- END ----------------------------\\ \n";
}

void MM::Arena::ArenaSnapshotToStream(std::ofstream& stream)
{
    stream << "\n//---------------------------- ARENA-PRINTING ----------------------------\\ \n";
    float percentage = (m_UsedBytes / (float)m_CapacityBytes) * 100;
    stream << "Arena: " << m_UsedBytes << "/" << m_CapacityBytes << " - " << percentage << "%";
    for (auto& pair : map)
    {
        reinterpret_cast<Pool*>(pair.second)->PoolSpapshotToStream(stream);
    }
    stream << "\n//--------------------------------- END ----------------------------\\ \n";
}

void MM::Arena::ArenaSnapshot(char* buffer)
{
    buffer += std::sprintf(buffer, "//---------------------------- ARENA-PRINTING ----------------------------\\\n");
    float percentage = (m_UsedBytes / (float)m_CapacityBytes) * 100;
    buffer += std::sprintf(buffer, "Arena: %d/%d - %f%\n", static_cast<int>(m_UsedBytes), static_cast<int>(m_CapacityBytes), static_cast<float>(percentage));
    for (auto& pair : map)
    {
        reinterpret_cast<Pool*>(pair.second)->PoolSpapshot(buffer);
    }
    buffer += std::sprintf(buffer, "//--------------------------------- END ----------------------------\\ \n");
}

//======================================================================
// Pool
MM::Pool::Pool(unsigned char* ptr, uint32_t slotSize, uint8_t slotCount)
    : m_NumChunks(0)
    , m_SlotSize(slotSize)
    , m_SlotsCount(slotCount)
    , m_HeadChunk(nullptr)
{
    MM_ASSERT(m_SlotSize > 0);
    MM_ASSERT(m_SlotsCount > 0 && m_SlotsCount <= 64);

    MM_PRINT("--[CREATED]\n");
    MM_PRINT("  Pool{" << std::to_string(m_SlotSize) << ", " << std::to_string(static_cast<uint32_t>(m_SlotsCount)) << "} has been created\n ")

    m_HeadChunk = new (ptr) Chunk(ptr + sizeof(Chunk), m_SlotSize, m_SlotsCount);

    MM_ASSERT(m_HeadChunk);

    ++m_NumChunks;
}

MM::Pool::~Pool()
{
    m_HeadChunk->~Chunk();
}

unsigned char* MM::Pool::AcquireSlotFromPool()
{
    return m_HeadChunk->TryAcquireSlotInChunkChain();
}

void MM::Pool::Extend(MM::Chunk* chunk)
{
    m_HeadChunk->SetNextChunkRecursive(chunk);
    m_NumChunks++;
}

void MM::Pool::Print()
{
    std::cout << "\nPrinting Pool{" << m_SlotSize << "," << m_SlotsCount << "}: Chunks: " << std::to_string(m_NumChunks);
    m_HeadChunk->Print(0);
}

void MM::Pool::PoolSpapshotToStream(std::ofstream& stream)
{
    stream << "\nPrinting Pool{" << this << "}{" << m_SlotSize << ", " << m_SlotsCount << "} : Chunks: " << std::to_string(m_NumChunks);
    m_HeadChunk->ChunkSnapshotToStream(0, stream);
}

void MM::Pool::PoolSpapshot(char*& buffer)
{
    buffer += std::sprintf(buffer, "Printing Pool{%p}{%d, %d}, Chunks: %d\n", this, m_SlotSize, m_SlotsCount, m_NumChunks);
    m_HeadChunk->ChunkSnapshot(0, buffer);
}

bool MM::Pool::FreeSlot(unsigned char* ptr)
{
    auto chunk = m_HeadChunk->IsPtrToBlockIsInChunkRecursive(ptr);
    if (chunk)
    {
        chunk->FreeChunkSlot(ptr);
        return true;
    }
    else
    {
        MM_DEBUG_BREAK(true);
        return false;
    }
}

bool MM::Pool::IsInChunkChain(const unsigned char* const ptr) const
{
    return m_HeadChunk->IsPtrToBlockIsInChunkRecursive(ptr);
}

//======================================================================
// Chunk
MM::Chunk::Chunk(unsigned char* ptr, uint32_t slotSize, uint8_t slotCount)
    : m_SlotSize(slotSize)
    , m_SlotsCount(slotCount)
    , m_CapacityBytes(PAGE_SIZE)
    , m_UsedBytes(0)
    , m_Head(nullptr)
    , m_Tail(nullptr)
    , next(nullptr)
    , bitmap(m_SlotsCount)
{
    MM_PRINT("--[CREATED]\n")
    MM_PRINT("  Chunk{" << m_SlotSize << ", " << m_SlotsCount << "} has been created")

    m_Head = ptr;
    m_Tail = m_Head + m_CapacityBytes;
    MM_PRINT(", Head{" << static_cast<void*>(m_Head) << "}, Tail{" << static_cast<void*>(m_Tail) << "}\n")
}

MM::Chunk::~Chunk()
{
    if (next)
        next->~Chunk();
}

unsigned char* MM::Chunk::TryAcquireSlotInChunkChain()
{
    if (!bitmap.IsFull())
    {
        uint32_t oldCapacity = m_UsedBytes;
        m_UsedBytes += m_SlotSize;

        MM_DEBUG_BREAK(m_UsedBytes > m_CapacityBytes)

        uint32_t freeSlotNew = 0;
        bitmap.ScanFirstFreeForward(&freeSlotNew);

        uint64_t oldBitmap = bitmap.Get();

        MM_PRINT("--[AcquireSlot]\n")
        MM_PRINT("   Chunk{" << this << "} {" << m_SlotSize << " / " << m_SlotsCount << "} old capacity : " << std::to_string(oldCapacity)
                             << ", new capacity: " << std::to_string(m_UsedBytes) << std::endl)
        MM_PRINT("   Slot: " << std::to_string(freeSlotNew) << ", Bitmap before: " << bitmap)

        bitmap.SetBit(freeSlotNew);

        MM_PRINT(", bitmap after: " << bitmap << std::endl)

        MM_DEBUG_BREAK(oldBitmap == bitmap.Get())

        unsigned char* nextPtr = m_Head + (freeSlotNew * m_SlotSize);

        MM_ASSERT(nextPtr < m_Tail);
        return nextPtr;
    }

    if (next)
    {
        return next->TryAcquireSlotInChunkChain();
    }

    return nullptr;
}

void MM::Chunk::SetNextChunkRecursive(Chunk* nextChunk)
{
    MM_ASSERT(nextChunk);

    if (!next)
        next = nextChunk;
    else
        next->SetNextChunkRecursive(nextChunk);
}

MM::Chunk* MM::Chunk::IsPtrToBlockIsInChunkRecursive(const unsigned char* const ptr)
{
    if (ptr < m_Tail && ptr >= m_Head)
    {
        MM_PRINT("IsPtrToBlockIsInChunkRecursive, this{" << this << "}, ptr{" << static_cast<const void*>(ptr) << "}\n")
        return this;
    }

    if (next)
    {
        MM_PRINT("Next IsPtrToBlockIsInChunkRecursive, this{" << this << "}, ptr{" << static_cast<const void*>(ptr) << "}\n")
        return next->IsPtrToBlockIsInChunkRecursive(ptr);
    }
    else
        return nullptr;
}

void MM::Chunk::FreeChunkSlot(unsigned char* ptr)
{
    MM_DEBUG_BREAK(m_UsedBytes == 0)

    uint32_t clearBit = (ptr - m_Head) / m_SlotSize;
    uint32_t old_m_UsedBytes = m_UsedBytes;
    uint64_t old_bitmap = bitmap.Get();

    MM_PRINT("--[Slot Free]\n")
    MM_PRINT("   Chunk{" << this << "} {" << m_SlotSize << " / " << m_SlotsCount << "}, bit : " << std::to_string(clearBit) << ", old bitmap:" << bitmap)

    bitmap.ClearBit(clearBit);

    m_UsedBytes -= m_SlotSize;

    MM_PRINT(", bitmap after: " << bitmap << " old used: " << std::to_string(old_m_UsedBytes) << std::endl)

    MM_DEBUG_EXPRESSION({
        if (bitmap.UsedBits() != m_UsedBytes / m_SlotSize)
        {
            std::cout << "Used bits aren't same as used: " << std::to_string(m_UsedBytes) << " slot size: " << std::to_string(m_SlotSize)
                      << ", bitmap bits:" << std::to_string(bitmap.UsedBits()) << ",chunk address : " << this << "\n ";
            __debugbreak();
        }
    })

    MM_DEBUG_BREAK(m_UsedBytes < 0)
}

void MM::Chunk::PrintBucket()
{
    const char emptyCell = '-';
    const char occupiedCell = 'x';
    const int cellsPerRow = 10;

    const uint32_t numCells = m_CapacityBytes / m_SlotSize;

    std::cout << "\nBucket View (" << m_UsedBytes << "/" << m_CapacityBytes << ")\n";

    uint32_t filled = m_UsedBytes / m_SlotSize;
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
void MM::Chunk::Print(uint32_t chunkNum)
{
    constexpr const char* Reset = "\033[0m";
    constexpr const char* FG_Black = "\033[30m";
    constexpr const char* FG_White = "\033[97m";
    constexpr const char* BG_Red = "\033[41m";
    constexpr const char* BG_Green = "\033[42m";

    static constexpr char emptyCell = ' ';
    static constexpr char occupiedCell = 'x';
    static constexpr uint32_t cellsPerRow = 40;

    std::cout << "\nChunk_" << chunkNum << "{" << this << "}" << "{" << m_SlotSize << ", " << m_SlotsCount << "}: " << m_UsedBytes << "/" << m_CapacityBytes
              << "\n";

    const uint32_t numCells = m_CapacityBytes / m_SlotSize;
    const uint32_t maxIndex = numCells ? numCells - 1 : 0;
    const int width = (maxIndex > 0) ? static_cast<int>(std::log10(maxIndex)) + 1 : 1;
    const uint32_t numRows = (numCells + cellsPerRow - 1) / cellsPerRow;

    for (uint32_t row = 0; row < numRows; ++row)
    {
        const uint32_t start = row * cellsPerRow;
        const uint32_t end = std::min(start + cellsPerRow, numCells);

        for (uint32_t i = start; i < end; ++i) std::cout << '[' << std::setw(width) << i << "] ";
        std::cout << "\n";

        for (uint32_t i = start; i < end; ++i)
        {
            const bool isOccupied = bitmap.IsBitOccupied(i);

            if (isOccupied)
                std::cout << FG_White << BG_Red << "  X " << Reset << " ";
            else
                std::cout << FG_Black << BG_Green << "  O " << Reset << " ";
        }

        std::cout << "\n\n";
    }
    // PrintBucket();

    if (next)
        next->Print(++chunkNum);
}
void MM::Chunk::ChunkSnapshotToStream(uint32_t chunkNum, std::ofstream& stream)
{
    std::string strUpper;
    std::string strDown;
    strUpper.reserve(160);
    strDown.reserve(160);

    stream << "\nChunk_" << chunkNum << "{" << this << "}{" << m_SlotSize << ", " << m_SlotsCount << "}: " << m_UsedBytes << "/" << m_CapacityBytes << "\n";

    for (size_t i = 0; i < m_SlotsCount; i++)
    {
        PrintSlot slot(i, !bitmap.IsBitOccupied(i));
        // slot.ToStream(strUpper, strDown);
        slot.ToStreanMinimalistic(strUpper);
    }
    stream << strUpper << '\n';
    stream << strDown << '\n';

    if (next)
        next->ChunkSnapshotToStream(++chunkNum, stream);
}
void MM::Chunk::ChunkSnapshot(uint32_t chunkNum, char*& buffer)
{
    std::string strUpper;
    std::string strDown;
    strUpper.reserve(160);
    strDown.reserve(160);

    buffer += std::sprintf(buffer, "Chunk_%d{%p}{%d, %d}{%d/%d}\n", chunkNum, this, m_SlotSize, m_SlotsCount, m_UsedBytes, m_CapacityBytes);

    for (size_t i = 0; i < m_SlotsCount; i++)
    {
        PrintSlot slot(i, !bitmap.IsBitOccupied(i));
        slot.ToStream(strUpper, strDown);
        // slot.ToStreanMinimalistic(strUpper);
    }
    buffer += std::sprintf(buffer, "%s", strUpper.c_str());
    buffer += std::sprintf(buffer, "%s", "\n");
    buffer += std::sprintf(buffer, "%s", strDown.c_str());
    buffer += std::sprintf(buffer, "%s", "\n");

    if (next)
    {
        next->ChunkSnapshot(++chunkNum, buffer);
    }
}

//======================================================================
// Chunk::PrintSlot
MM::Chunk::PrintSlot::PrintSlot(uint32_t slotNum, bool isFree)
    : m_IsFree(isFree)
    , m_Slot_num(slotNum)
{
}
void MM::Chunk::PrintSlot::ToStream(std::string& upper, std::string& down)
{
    upper += '[';
    down += ' ';

    if (m_Slot_num <= 9)
    {
        upper += std::to_string(m_Slot_num);
    }
    else if (m_Slot_num > 9 && m_Slot_num < 100)
    {
        upper += std::to_string(m_Slot_num);
        down += " ";
    }
    if (m_IsFree)
        down += '0';
    else
        down += 'X';

    upper += "] ";
    down += "  ";
}
void MM::Chunk::PrintSlot::ToStreanMinimalistic(std::string& string)
{
    if (m_IsFree)
        string += '0';
    else
        string += 'X';
}