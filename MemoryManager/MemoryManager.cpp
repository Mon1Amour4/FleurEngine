#include "MemoryManager.h"

//======================================================================
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
    assert(capacity > 0 && pageSize > 0, minSlotSize > 0);
    assert(arenaSize <= capacity);

    m_Head = reinterpret_cast<unsigned char*>(malloc(capacity));

    assert(m_Head);

    std::cout << "Memory Manager has allocated " << std::to_string(capacity) << "bytes\n";

    m_LocalArena = new (static_cast<void*>(m_Head)) Arena(m_Head + sizeof(Arena), arenaSize - sizeof(Arena), pageSize, minSlotSize);

    assert(m_LocalArena);

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
void MM::MemoryManager::SaveSnapshotToFile(std::string_view fileName)
{
    std::ofstream myFile;
    myFile.open(fileName.data(), std::ios::app);

    if (myFile.good())
        m_LocalArena->ArenaSnapshotToStream(myFile);

    myFile.close();
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
MM::Arena::Arena(unsigned char* ptr, size_t capacity, size_t pageSize, uint32_t minSlotSize)
    : m_PageSize(pageSize)
    , m_MinSlotSize(minSlotSize)
    , m_CapacityBytes(capacity)
    , m_UsedBytes(0)
    , m_Head(ptr)
{
    assert(m_Head);

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
    size_t updatedUsed = m_UsedBytes + m_PageSize;
    if (updatedUsed <= m_CapacityBytes)
    {
        map[slotSize] = new (m_Current) Pool(m_Current + sizeof(Pool), slotSize, m_PageSize / slotSize);
        m_Current += m_PageSize;
        m_UsedBytes += m_PageSize;
        return static_cast<Pool*>(map[slotSize]);
    }
    else
    {
        // Not enought space in current arena
        assert(false);
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
    unsigned char* requestedPtr = m_Current + m_PageSize;
    if (requestedPtr < m_Tail)
    {
        // Arena has enought space for new chunk, give it
        unsigned char* prevPtr = m_Current;
        m_Current = requestedPtr;
        return new (prevPtr) Chunk(prevPtr + sizeof(Chunk), slotSize, slotsCount);
    }
    else
    {
        assert(false);
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

//======================================================================
// Pool
MM::Pool::Pool(unsigned char* ptr, uint32_t slotSize, uint8_t slotCount)
    : m_NumChunks(0)
    , m_SlotSize(slotSize)
    , m_SlotsCount(slotCount)
    , m_HeadChunk(nullptr)
{
    assert(m_SlotSize > 0);
    assert(m_SlotsCount > 0 && m_SlotsCount <= 64);

    std::cout << "Pool{" << m_SlotSize << "," << m_SlotsCount << "} has been created\n";

    m_HeadChunk = new (ptr) Chunk(ptr + sizeof(Chunk), m_SlotSize, m_SlotsCount);

    assert(m_HeadChunk);

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
    stream << "\nPrinting Pool{" << m_SlotSize << "," << m_SlotsCount << "}: Chunks: " << std::to_string(m_NumChunks);
    m_HeadChunk->ChunkSnapshotToStream(0, stream);
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
        __debugbreak();
        return false;
    }
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
    std::cout << "Chunk{" << m_SlotSize << "," << m_SlotsCount << "} has been created\n";

    m_Head = ptr;
    m_Tail = m_Head + m_CapacityBytes;
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

        if (m_UsedBytes > m_CapacityBytes)
            __debugbreak();

        uint8_t slot = oldCapacity / m_SlotSize;
        bitmap.SetBit(slot);
        unsigned char* nextPtr = m_Head + (slot * m_SlotSize);
        assert(nextPtr < m_Tail);
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
    assert(nextChunk);

    if (!next)
        next = nextChunk;
    else
        next->SetNextChunkRecursive(nextChunk);
}

MM::Chunk* MM::Chunk::IsPtrToBlockIsInChunkRecursive(unsigned char* ptr)
{
    if (ptr < m_Tail && ptr >= m_Head)
        return this;

    if (next)
        return next->IsPtrToBlockIsInChunkRecursive(ptr);
    else
        return nullptr;
}

void MM::Chunk::FreeChunkSlot(unsigned char* ptr)
{
    bitmap.ClearBit((ptr - m_Head) / m_SlotSize);
    m_UsedBytes -= m_SlotSize;
    std::cout << "Slot has freed\n";

    if (bitmap.UsedBits() != m_UsedBytes / m_SlotSize)
        __debugbreak();

    if (m_UsedBytes < 0)
        __debugbreak();
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

    std::cout << "\nChunk_" << chunkNum << "{" << m_SlotSize << ", " << m_SlotsCount << "}: " << m_UsedBytes << "/" << m_CapacityBytes << "\n";

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
    struct PrintSlot
    {
        PrintSlot(uint32_t slotNum, bool isFree)
            : m_IsFree(isFree)
            , m_Slot_num(slotNum) {};

        void ToStream(std::string& upper, std::string& down)
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
        void ToStreanMinimalistic(std::string& string)
        {
            if (m_IsFree)
                string += '0';
            else
                string += 'X';
        }

        bool m_IsFree;
        uint32_t m_Slot_num;
    };

    std::string strUpper;
    std::string strDown;
    strUpper.reserve(160);
    strDown.reserve(160);

    stream << "\nChunk_" << chunkNum << "{" << m_SlotSize << ", " << m_SlotsCount << "}: " << m_UsedBytes << "/" << m_CapacityBytes << "\n";

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
