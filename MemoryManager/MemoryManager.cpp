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
    MM_ASSERT(m_Capacity > 0 && m_PageSize > 0 && m_MinSlotSize > 0);
    MM_ASSERT(m_ArenaSize <= m_Capacity);

    m_Head = reinterpret_cast<unsigned char*>(malloc(capacity));

    MM_ASSERT(m_Head);

    MM_PRINT("Memory Manager has allocated " << std::to_string(capacity) << " bytes\n");

    unsigned char* arenaHeadPtr = m_Head + sizeof(Arena);
    m_LocalArena = new (m_Head) Arena(arenaHeadPtr, arenaSize, pageSize, minSlotSize);

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
    uint32_t bufferSize = 1024 * 1024;
    char* buffer = new char[bufferSize];
    buffer[bufferSize - 1] = '\0';
    char* tmp = buffer;
    tmp += sprintf_s(tmp, bufferSize, "Iteration: %d\n", iteration);

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
    , m_SmallObjectsCurrent(nullptr)
{
    MM_ASSERT(m_Head);

    m_Current = m_Head;
    m_Tail = m_Head + (m_CapacityBytes);

    uint32_t poolsCount = CountSlots(m_PageSize, 16) - 1;
    m_StaticOffset = poolsCount * sizeof(Pool);

    uint32_t offset = sizeof(Pool);
    unsigned char* chunksBase = m_Head + m_StaticOffset;
    uint32_t chunkStride = sizeof(Chunk) + m_PageSize;
    for (size_t i = 0; i < poolsCount; i++)
    {
        uint32_t poolSize = m_MinSlotSize + (8 * i);
        unsigned char* poolPtr = m_Head + offset * i;
        Pool* pool = new (poolPtr) Pool(poolSize, m_PageSize / poolSize);
    }

    m_UsedBytes += m_StaticOffset;
    m_Current += m_UsedBytes;
}
MM::Arena::~Arena()
{
    // TODO
}
MM::Arena MM::Arena::operator=(const Arena& other)
{
    return Arena(other.m_Head, other.m_CapacityBytes, other.m_PageSize, other.m_MinSlotSize);
};

MM::Pool* MM::Arena::GetPool(uint32_t slotSize)
{
    uint32_t multiplier = slotSize / 8 - 2;
    return reinterpret_cast<Pool*>(m_Head + (sizeof(Pool) * multiplier));
}

MM::Chunk* MM::Arena::FindChunk(unsigned char* ptrToSlot)
{
    uint32_t chunkStride = sizeof(Chunk) + m_PageSize;

    unsigned char* endOfStatic = m_Head + m_StaticOffset;
    uint32_t diff = ptrToSlot - endOfStatic;
    uint32_t steps = diff / chunkStride;
    Chunk* chunk = reinterpret_cast<Chunk*>(endOfStatic + (steps * chunkStride));

    MM_DEBUG_BREAK(!chunk->IsValid());
    return chunk;
}
MM::Chunk* MM::Arena::TryToGetNewChunk(MM::Pool* pool, uint32_t slotSize, uint32_t slotsCount)
{
    uint32_t requestedSize = m_PageSize + sizeof(Chunk);
    if (m_UsedBytes + requestedSize <= m_CapacityBytes)
    {
        // Arena has enought space for new chunk, give it
        unsigned char* prevPtr = m_Current;
        m_Current += requestedSize;
        m_UsedBytes += requestedSize;
        MM_DEBUG_EXPRESSION({
            Chunk* newChunk = new (prevPtr) Chunk(prevPtr + sizeof(Chunk), slotSize, slotsCount);
            unsigned char* newChunkChar = reinterpret_cast<unsigned char*>(newChunk);

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
    uint32_t offset = m_PageSize + sizeof(Pool) + sizeof(Chunk);
    for (size_t i = 0; m_MinSlotSize << i <= m_PageSize; i++)
    {
        uint32_t size = m_MinSlotSize << i;
        reinterpret_cast<Pool*>(m_Head + offset * i)->Print();
    }

    std::cout << "\n//--------------------------------- END ----------------------------\\ \n";
}
void MM::Arena::ArenaSnapshotToStream(std::ofstream& stream)
{
    stream << "\n//---------------------------- ARENA-PRINTING ----------------------------\\ \n";
    float percentage = (m_UsedBytes / (float)m_CapacityBytes) * 100;
    stream << "Arena: " << m_UsedBytes << "/" << m_CapacityBytes << " - " << percentage << "%";

    uint32_t offset = m_PageSize + sizeof(Pool) + sizeof(Chunk);
    for (size_t i = 0; m_MinSlotSize << i <= m_PageSize; i++)
    {
        uint32_t size = m_MinSlotSize << i;
        reinterpret_cast<Pool*>(m_Head + offset * i)->PoolSpapshotToStream(stream);
    }

    stream << "\n//--------------------------------- END ----------------------------\\ \n";
}
void MM::Arena::ArenaSnapshot(char* buffer)
{
    buffer += std::sprintf(buffer, "//---------------------------- ARENA-PRINTING ----------------------------\\\n");
    float percentage = (m_UsedBytes / (float)m_CapacityBytes) * 100;
    buffer += std::sprintf(buffer, "Arena: %d/%d - %f%\n", static_cast<int>(m_UsedBytes), static_cast<int>(m_CapacityBytes), static_cast<float>(percentage));

    uint32_t poolsCount = CountSlots(m_PageSize, 16) - 1;
    uint32_t chunks = 0;
    uint32_t offset = sizeof(Pool);

    for (size_t i = 0; i < poolsCount; i++)
    {
        uint32_t poolSize = m_MinSlotSize + (8 * i);
        Pool* pool = reinterpret_cast<Pool*>(m_Head + offset * i);
        // pool->PoolSpapshot(buffer);
        chunks += pool->Chunks();
    }
    uint32_t chunkStride = sizeof(Chunk) + m_PageSize;
    for (size_t i = 0; i < chunks; i++)
    {
        Chunk* chunk = reinterpret_cast<Chunk*>((m_Head + m_StaticOffset) + (chunkStride * i));
        chunk->ChunkSnapshot(i, buffer);
    }


    buffer += std::sprintf(buffer, "//--------------------------------- END ----------------------------\\ \n");
}


//======================================================================
// Pool
MM::Pool::Pool(uint32_t slotSize, uint32_t slotCount)
    : m_NumChunks(0)
    , m_SlotSize(slotSize)
    , m_SlotsCount(slotCount)
    , m_HeadOffsetBytes(INVALID_OFFSET)
{
    MM_ASSERT(m_SlotSize > 0);
    MM_ASSERT(m_SlotsCount > 0);

    MM_PRINT("--[CREATED]\n");
    MM_PRINT("  Pool{" << std::to_string(m_SlotSize) << ", " << std::to_string(static_cast<uint32_t>(m_SlotsCount)) << "} has been created\n ")
}
MM::Pool::~Pool()
{
    reinterpret_cast<Chunk*>(reinterpret_cast<unsigned char*>(this) + m_HeadOffsetBytes)->~Chunk();
}

bool MM::Pool::AcquireSlotFromPool(unsigned char*& outPtr)
{
    if (m_HeadOffsetBytes == INVALID_OFFSET)
        return false;

    Chunk* currentHead = reinterpret_cast<Chunk*>(TOCHARPTR(this) + m_HeadOffsetBytes);
    if (currentHead->AcquireSlot(outPtr))
        return true;
    else
    {
        // We've acquired the last free slot, now we need to get next free chunk and set it as Head
        Chunk* nextHead = currentHead->GetNext();
        currentHead->SetNext(nullptr);
        currentHead->SetPrev(nullptr);
        if (nextHead)
        {
            nextHead->SetPrev(nullptr);
            m_HeadOffsetBytes = TOCHARPTR(nextHead) - TOCHARPTR(this);
        }
        else
            m_HeadOffsetBytes = INVALID_OFFSET;

        return outPtr != nullptr;
    }
}
bool MM::Pool::FreeSlot(Chunk* chunk, unsigned char* ptrToSlot)
{
    bool isChunkEmpty = false;
    if (chunk->FreeChunkSlot(ptrToSlot, &isChunkEmpty))
    {
        // We need to add this chunk to a free list

        if (m_HeadOffsetBytes == INVALID_OFFSET)
            m_HeadOffsetBytes = TOCHARPTR(chunk) - TOCHARPTR(this);
        else
        {
            Chunk* currentHead = reinterpret_cast<Chunk*>(TOCHARPTR(this) + m_HeadOffsetBytes);
            m_HeadOffsetBytes = TOCHARPTR(chunk) - TOCHARPTR(this);

            //  Before:         [current]⇄[other]⇄[other]
            //  After:  [freed]⇄[current]⇄[other]⇄[other]
            currentHead->SetPrev(chunk);
            chunk->SetNext(currentHead);
        }
        return false;
    }
    else
    {
        // Chunk had at least one free slot
        // This Chunk is in linked list

        if (isChunkEmpty)
        {
            if (m_NumChunks > 1)
            {
                // Do not free the last free Chunk
                // Need to fix linked list
                Chunk* next = chunk->GetNext();
                Chunk* prev = chunk->GetPrev();
                if (prev)
                    prev->SetNext(next);
                if (next)
                    next->SetPrev(prev);

                chunk->SetNext(nullptr);
                chunk->SetPrev(nullptr);

                --m_NumChunks;

                Chunk* currentHead = reinterpret_cast<Chunk*>(TOCHARPTR(this) + m_HeadOffsetBytes);
                if (chunk == currentHead)
                    m_HeadOffsetBytes = TOCHARPTR(next) - TOCHARPTR(this);

                return true;
            }
        }
        return false;
    }
}

void MM::Pool::Extend(MM::Chunk* chunk)
{
    MM_PRINT("Pool{" << this << "} extend by chunk{" << &*chunk << "}\n");

    m_HeadOffsetBytes = reinterpret_cast<unsigned char*>(chunk) - reinterpret_cast<unsigned char*>(this);
    m_NumChunks++;
}

void MM::Pool::Print()
{
    std::cout << "\nPrinting Pool{" << m_SlotSize << "," << m_SlotsCount << "}: Chunks: " << std::to_string(m_NumChunks);

    Chunk* chunk = reinterpret_cast<Chunk*>((reinterpret_cast<unsigned char*>(this) + m_HeadOffsetBytes));
    chunk->Print(0);
}
void MM::Pool::PoolSpapshotToStream(std::ofstream& stream)
{
    stream << "\nPrinting Pool{" << this << "}{" << m_SlotSize << ", " << m_SlotsCount << "} : Chunks: " << std::to_string(m_NumChunks);
    Chunk* chunk = reinterpret_cast<Chunk*>((reinterpret_cast<unsigned char*>(this) + m_HeadOffsetBytes));
    chunk->ChunkSnapshotToStream(0, stream);
}
void MM::Pool::PoolSpapshot(char*& buffer)
{
    buffer +=
        std::sprintf(buffer, "Printing Pool{%p}{%d, %d}, Chunks: %d, free chunk: %-18p\n", this, m_SlotSize, m_SlotsCount, m_NumChunks, m_HeadOffsetBytes);
}


//======================================================================
// Chunk
MM::Chunk::Chunk(unsigned char* ptr, uint32_t slotSize, uint32_t slotCount)
    : m_SlotSize(slotSize)
    , m_CapacityBytes(m_SlotSize * slotCount)
    , m_UsedBytes(0)
    , m_FreeSlot(0)
    , m_NextChunkOffsetInChunkStride(0)
    , m_PrevChunkOffsetInChunkStride(0)
{
    LOCAL_HEAD(this, Chunk);

    MM_DEBUG_BREAK(m_SlotSize * slotCount > 4096);
    MM_PRINT("--[CREATED]\n")
    MM_PRINT("  Chunk{" << this << "}{" << m_SlotSize << ", " << slotCount << "} has been created")

    MM_PRINT(", Head{" << static_cast<void*>(localHead) << "}, Tail{" << static_cast<void*>(localHead + m_CapacityBytes) << "}\n")

    // TODO A place for optimization
    for (size_t i = 0; i < slotCount; i++)
    {
        uint32_t* slotPtr = reinterpret_cast<uint32_t*>(localHead + (m_SlotSize * i));
        if (i == slotCount - 1)
            *slotPtr = II_NULL_INDEX;
        else
            *slotPtr = i + 1;
    }
}

MM::Chunk::~Chunk()
{
}

bool MM::Chunk::AcquireSlot(unsigned char*& outPtr)
{
    uint32_t oldCapacity = m_UsedBytes;
    m_UsedBytes += m_SlotSize;

    MM_DEBUG_BREAK(m_UsedBytes > m_CapacityBytes);

    if (m_FreeSlot == II_NULL_INDEX)
    {
        // No Space in Chunk at all, we cant have this case because Pool stores ptr to chunk that has at least one free slot
        MM_DEBUG_BREAK(true);
        return false;
    }
    else
    {
        LOCAL_HEAD(this, Chunk);
        outPtr = localHead + (m_FreeSlot * m_SlotSize);

        uint32_t nextIdx = *reinterpret_cast<uint32_t*>(outPtr);
        if (nextIdx == II_NULL_INDEX)
        {
            // Slot was last free slot in this chunk
            m_FreeSlot = II_NULL_INDEX;
            return false;
        }
        else
        {
            m_FreeSlot = nextIdx;
            return true;
        }
    }
}

bool MM::Chunk::FreeChunkSlot(unsigned char* ptrToSLot, bool* isEmpty)
{
    LOCAL_HEAD(this, Chunk);

    MM_DEBUG_BREAK(m_UsedBytes == 0);
    MM_DEBUG_BREAK(ptrToSLot - localHead == 4096);

    bool wasFull = m_UsedBytes == m_CapacityBytes;
    m_UsedBytes -= m_SlotSize;
    uint32_t deallocatedIdx = (ptrToSLot - localHead) / m_SlotSize;
    if (wasFull)
    {
        m_FreeSlot = deallocatedIdx;
        uint32_t* slot = reinterpret_cast<uint32_t*>(localHead + (m_FreeSlot * m_SlotSize));
        *slot = II_NULL_INDEX;
    }
    else
    {
        uint32_t currentIdx = *reinterpret_cast<uint32_t*>(localHead + (m_FreeSlot * m_SlotSize));
        m_FreeSlot = deallocatedIdx;
        uint32_t* newSlot = reinterpret_cast<uint32_t*>(localHead + m_FreeSlot * m_SlotSize);
        *newSlot = currentIdx;
    }

    MM_PRINT("--[Slot Free]\n")
    MM_PRINT("--Ptr to slot: " << static_cast<void*>(ptrToSLot) << "\n")
    MM_DEBUG_BREAK(m_UsedBytes > 10000)

    *isEmpty = m_UsedBytes == 0;
    return wasFull;
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

    std::cout << "\nChunk_" << chunkNum << "{" << this << "}" << "{" << m_SlotSize << ", " << m_CapacityBytes / m_SlotSize << "}: " << m_UsedBytes << "/"
              << m_CapacityBytes << "\n";

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
            // TODO: FIX const bool isOccupied = bitmap.IsBitOccupied(i);

            // TODO: FIX
            /* if (isOccupied)
                 std::cout << FG_White << BG_Red << "  X " << Reset << " ";
             else
                 std::cout << FG_Black << BG_Green << "  O " << Reset << " ";*/
        }

        std::cout << "\n\n";
    }
    // PrintBucket();
}
void MM::Chunk::ChunkSnapshotToStream(uint32_t chunkNum, std::ofstream& stream)
{
    std::string strUpper;
    std::string strDown;
    strUpper.reserve(160);
    strDown.reserve(160);

    stream << "\nChunk_" << chunkNum << "{" << this << "}{" << m_SlotSize << ", " << m_CapacityBytes / m_SlotSize << "}: " << m_UsedBytes << "/"
           << m_CapacityBytes << "\n";

    for (size_t i = 0; i < m_CapacityBytes / m_SlotSize; i++)
    {
        // TODO: FIX PrintSlot slot(i, !bitmap.IsBitOccupied(i));
        // slot.ToStream(strUpper, strDown);
        // TODO: FIX slot.ToStreanMinimalistic(strUpper);
    }
    stream << strUpper << '\n';
    stream << strDown << '\n';
}
void MM::Chunk::ChunkSnapshot(uint32_t chunkNum, char*& buffer)
{
    char sign = 0;
    if (m_UsedBytes == 0)
        sign = '0';
    else if (m_UsedBytes > 0 && m_UsedBytes < m_CapacityBytes)
        sign = '-';
    else
        sign = 'x';

    Chunk* nextChunk = GetNext();
    Chunk* nextNextChunk = nullptr;
    if (nextChunk)
    {
        nextNextChunk = nextChunk->GetNext();
    }
    buffer += std::sprintf(buffer, "|%-3d|%-18p|%4d/%-4d|%4d/%-d|%c|%-18p|%-18p|\n", chunkNum, this, m_SlotSize, m_CapacityBytes / m_SlotSize, m_UsedBytes,
                           m_CapacityBytes, sign, nextChunk, nextNextChunk);
}

void MM::Chunk::SetNext(Chunk* next)
{
    if (next == this)
        return;

    if (!next)
    {
        m_NextChunkOffsetInChunkStride = 0;
        return;
    }
    int nextPtrDiff = TOCHARPTR(next) - TOCHARPTR(this);
    m_NextChunkOffsetInChunkStride = nextPtrDiff / static_cast<int>(CHUNK_STRIDE);
}

void MM::Chunk::SetPrev(Chunk* prev)
{
    if (prev == this)
        return;

    if (!prev)
    {
        m_PrevChunkOffsetInChunkStride = 0;
        return;
    }
    int prevPtrDiff = TOCHARPTR(prev) - TOCHARPTR(this);
    m_PrevChunkOffsetInChunkStride = prevPtrDiff / static_cast<int>(CHUNK_STRIDE);
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