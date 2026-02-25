#pragma once
#include "MemoryDefinitions.hpp"
#include "PageAllocator.hpp"
#include <cmath>

#define CHUNK_PAYLOAD_SIZE(pageSize) (pageSize - sizeof(Chunk))
#pragma region Chunk
//======================================================================
struct Chunk
{
public:
    Chunk(uint32_t slotSize, uint32_t slotCount)
        : m_SlotSize(slotSize)
        , m_CapacityBytes(m_SlotSize * slotCount)
        , m_UsedBytes(0)
        , m_FreeSlot(0)
        , m_NextChunkOffsetInChunkStride(0)
        , m_PrevChunkOffsetInChunkStride(0)
    {
        LOCAL_HEAD(this, Chunk);

        MM_DEBUG_BREAK(m_CapacityBytes > 4096);
        MM_PRINT("--[CREATED]\n")
        MM_PRINT("  Chunk{" << this << "}{" << m_SlotSize << ", " << slotCount << "} has been created")

        MM_PRINT(", Head{" << static_cast<void*>(localHead) << "}, Tail{" << static_cast<void*>(localHead + m_CapacityBytes) << "}\n")

        // TODO A place for optimization
        for (uint32_t i = 0; i < slotCount; i++)
        {
            uint32_t* slotPtr = reinterpret_cast<uint32_t*>(localHead + (m_SlotSize * i));
            if (i == slotCount - 1)
                *slotPtr = II_NULL_INDEX;
            else
                *slotPtr = i + 1;
        }
    }
    ~Chunk()
    {
        // TODO
    }

    // True - there is at least one free chunk after acquisition
    // False - Chunk is full
    bool AcquireSlot(unsigned char*& outPtr)
    {
        MM_DEBUG_BREAK(!IsValid())

        m_UsedBytes += m_SlotSize;

        MM_DEBUG_BREAK(m_UsedBytes > m_CapacityBytes);
        MM_DEBUG_BREAK(!IsValid())
        if (m_FreeSlot == II_NULL_INDEX)
        {
            // No Space in Chunk at all, we cant have this case because Pool stores ptr to chunk that has at least one free slot
            MM_DEBUG_BREAK(m_FreeSlot == II_NULL_INDEX);
            return false;
        }
        else
        {
            LOCAL_HEAD(this, Chunk);
            outPtr = localHead + (m_FreeSlot * m_SlotSize);
            MM_DEBUG_BREAK(outPtr - localHead > 4096);

            MM_PRINT("--[Slot Aquired] 0x" << static_cast<void*>(outPtr) << ", Chunk" << "{" << m_SlotSize << "/" << m_CapacityBytes << "}" << "{0x"
                                           << static_cast<void*>(this) << "}," << "{" << ", Old capacity: " << oldCapacity << ", Used Bytes: " << m_UsedBytes
                                           << std::endl;);

            uint32_t nextIdx = *reinterpret_cast<uint32_t*>(outPtr);
            if (nextIdx == II_NULL_INDEX)
            {
                // Slot was last free slot in this chunk
                m_FreeSlot = II_NULL_INDEX;
                MM_DEBUG_BREAK(m_FreeSlot == II_NULL_INDEX && m_UsedBytes < m_CapacityBytes)
                return false;
            }
            else
            {
                m_FreeSlot = nextIdx;
                return true;
            }
        }
    }

    // True - Pool must add this chunk to free list
    // False - This chunk is already in a free list
    bool FreeChunkSlot(unsigned char* ptrToSLot, bool* isEmpty)
    {
        LOCAL_HEAD(this, Chunk);

        MM_DEBUG_BREAK(m_UsedBytes == 0);
        MM_DEBUG_BREAK(ptrToSLot - localHead == 4096);

        bool wasFull = m_UsedBytes == m_CapacityBytes;
        m_UsedBytes -= m_SlotSize;
        uint32_t deallocatedIdx = static_cast<uint32_t>((ptrToSLot - localHead) / m_SlotSize);
        if (wasFull)
        {
            MM_DEBUG_BREAK(deallocatedIdx == II_NULL_INDEX)

            m_FreeSlot = deallocatedIdx;
            uint32_t* slot = reinterpret_cast<uint32_t*>(localHead + (m_FreeSlot * m_SlotSize));
            *slot = II_NULL_INDEX;
        }
        else
        {
            MM_DEBUG_BREAK(m_FreeSlot == II_NULL_INDEX)

            uint32_t currentIdx = m_FreeSlot;
            m_FreeSlot = deallocatedIdx;
            MM_DEBUG_BREAK(deallocatedIdx == II_NULL_INDEX)
            uint32_t* newSlot = reinterpret_cast<uint32_t*>(localHead + m_FreeSlot * m_SlotSize);
            *newSlot = currentIdx;
        }

        MM_PRINT("--[Slot Free] " << "0x" << static_cast<void*>(ptrToSLot) << std::endl;)
        MM_DEBUG_BREAK(m_UsedBytes > 10000)

        *isEmpty = m_UsedBytes == 0;
        return wasFull;
    }

    void PrintBucket()
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
    void Print(uint32_t chunkNum)
    {
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

            std::cout << "\n\n";
        }
        // PrintBucket();
    }
    void ChunkSnapshotToStream(uint32_t chunkNum, std::ofstream& stream)
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
    void ChunkSnapshot(uint32_t chunkNum, char*& buffer)
    {
        std::string sign;
        if (m_UsedBytes == 0)
            sign = "Empty";
        else if (m_UsedBytes > 0 && m_UsedBytes < m_CapacityBytes)
            sign = "Partial";
        else
            sign = "Full";

        std::string next;
        Chunk* nextChunk = GetNext();
        if (!nextChunk)
            next = "None";
        else
        {
            next += "ox";
            std::ostringstream oss;
            oss << nextChunk;
            next = oss.str();
        }
        buffer += snprintf(buffer, 1024l * 1024l * 50, " |%-3d| 0x%-16p |%4d/%-4d|%4d/%-d|%-7s|\tnext{%-16s} |\n", chunkNum, this, m_SlotSize,
                           m_CapacityBytes / m_SlotSize, m_UsedBytes, m_CapacityBytes, sign.c_str(), next.c_str());
        if (nextChunk)
            nextChunk->ChunkSnapshot(++chunkNum, buffer);
    }

    void SetNext(Chunk* next)
    {
        if (next == this)
            return;

        if (!next)
        {
            m_NextChunkOffsetInChunkStride = 0;
            return;
        }
        int nextPtrDiff = static_cast<int>(TOCHARPTR(next) - TOCHARPTR(this));
        m_NextChunkOffsetInChunkStride = nextPtrDiff / static_cast<int>(4096);
    }
    void SetPrev(Chunk* prev)
    {
        if (prev == this)
            return;

        if (!prev)
        {
            m_PrevChunkOffsetInChunkStride = 0;
            return;
        }
        int prevPtrDiff = static_cast<int>(TOCHARPTR(prev) - TOCHARPTR(this));
        m_PrevChunkOffsetInChunkStride = prevPtrDiff / static_cast<int>(4096);
    }
    inline Chunk* GetNext()
    {
        if (m_NextChunkOffsetInChunkStride == 0)
            return nullptr;

        Chunk* chunk = reinterpret_cast<Chunk*>(TOCHARPTR(this) + (m_NextChunkOffsetInChunkStride * static_cast<int>(4096)));
        MM_DEBUG_BREAK(!chunk->IsValid());
        return chunk;
    }
    inline Chunk* GetPrev()
    {
        if (m_PrevChunkOffsetInChunkStride == 0)
            return nullptr;

        Chunk* chunk = reinterpret_cast<Chunk*>(TOCHARPTR(this) + (m_PrevChunkOffsetInChunkStride * static_cast<int>(4096)));
        MM_DEBUG_BREAK(!chunk->IsValid());
        return chunk;
    }

    inline bool IsValid() const
    {
        return m_UsedBytes <= 4096 && m_CapacityBytes > 0 && m_CapacityBytes <= 4096 && m_SlotSize > 0 && m_SlotSize <= 4096;
    }

private:
    const uint32_t m_SlotSize;
    const uint32_t m_CapacityBytes;

    uint32_t m_UsedBytes;

    uint32_t m_FreeSlot;

    // Offset: n * (sizeof(Chunk) + PageSize) bytes
    int m_NextChunkOffsetInChunkStride;
    int m_PrevChunkOffsetInChunkStride;

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
};
#pragma endregion

#pragma region Pool
//======================================================================
struct Pool
{
    Pool(uint32_t slotSize, uint32_t slotCount)
        : m_HeadOffsetBytes(INVALID_OFFSET)
        , m_NumChunks(0)
        , m_SlotSize(slotSize)
        , m_SlotsCount(slotCount)
    {
        MM_ASSERT(m_SlotSize > 0);
        MM_ASSERT(m_SlotsCount > 0);

        MM_PRINT("--[CREATED]\n");
        MM_PRINT("  Pool{" << std::to_string(m_SlotSize) << ", " << std::to_string(static_cast<uint32_t>(m_SlotsCount)) << "} has been created\n ")
    }
    ~Pool()
    {
        reinterpret_cast<Chunk*>(reinterpret_cast<unsigned char*>(this) + m_HeadOffsetBytes)->~Chunk();
    }

    // False - no space
    bool AcquireSlotFromPool(unsigned char*& outPtr)
    {
        if (m_HeadOffsetBytes == INVALID_OFFSET)
            return false;

        Chunk* currentHead = reinterpret_cast<Chunk*>(TOCHARPTR(this) + m_HeadOffsetBytes);
        MM_DEBUG_BREAK(!currentHead->IsValid());
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
                MM_DEBUG_EXPRESSION({
                    size_t localVar = TOCHARPTR(nextHead) - TOCHARPTR(this);
                    MM_DEBUG_BREAK(localVar > 4'294'967'295);
                })
                m_HeadOffsetBytes = static_cast<uint32_t>(TOCHARPTR(nextHead) - TOCHARPTR(this));
                MM_DEBUG_BREAK(m_HeadOffsetBytes > 4000000000);
            }
            else
                m_HeadOffsetBytes = INVALID_OFFSET;

            return outPtr != nullptr;
        }
    }

    void Extend(Chunk* chunk)
    {
        MM_PRINT("Pool{" << this << "} extend by chunk{" << &*chunk << "}\n");

        if (m_HeadOffsetBytes == INVALID_OFFSET)
        {
            MM_DEBUG_EXPRESSION({
                size_t localVar = TOCHARPTR(chunk) - TOCHARPTR(this);
                MM_DEBUG_BREAK(localVar > 4'294'967'295);
            })
            m_HeadOffsetBytes = static_cast<uint32_t>(reinterpret_cast<unsigned char*>(chunk) - reinterpret_cast<unsigned char*>(this));
        }
        else
        {
            Chunk* currentHead = reinterpret_cast<Chunk*>(TOCHARPTR(this) + m_HeadOffsetBytes);
            m_HeadOffsetBytes = static_cast<uint32_t>(TOCHARPTR(chunk) - TOCHARPTR(this));
            currentHead->SetPrev(chunk);
            chunk->SetNext(currentHead);
        }
        m_NumChunks++;
    }

    void Print()
    {
        std::cout << "\nPrinting Pool{" << m_SlotSize << "," << m_SlotsCount << "}: Chunks: " << std::to_string(m_NumChunks);

        Chunk* chunk = reinterpret_cast<Chunk*>((reinterpret_cast<unsigned char*>(this) + m_HeadOffsetBytes));
        chunk->Print(0);
    }
    void PoolSpapshotToStream(std::ofstream& stream)
    {
        stream << "\nPrinting Pool{" << this << "}{" << m_SlotSize << ", " << m_SlotsCount << "} : Chunks: " << std::to_string(m_NumChunks);
        Chunk* chunk = reinterpret_cast<Chunk*>((reinterpret_cast<unsigned char*>(this) + m_HeadOffsetBytes));
        chunk->ChunkSnapshotToStream(0, stream);
    }
    void PoolSpapshot(char*& buffer)
    {
        if (m_HeadOffsetBytes != INVALID_OFFSET)
        {
            Chunk* chunk = reinterpret_cast<Chunk*>(TOCHARPTR(this) + m_HeadOffsetBytes);
            MM_DEBUG_BREAK(!chunk->IsValid())
            /*buffer +=
                std::sprintf(buffer, "\nPrinting Pool{%p}{%d, %d}, Chunks: %d, free chunk: 0x%-18p\n", this, m_SlotSize, m_SlotsCount, m_NumChunks, chunk);*/
            chunk->ChunkSnapshot(0, buffer);
        }
    }

    // True - Chunk is empty
    bool FreeSlot(Chunk* chunk, unsigned char* ptrToSlot)
    {
        bool isChunkEmpty = false;
        if (chunk->FreeChunkSlot(ptrToSlot, &isChunkEmpty))
        {
            // We need to add this chunk to a free list

            if (m_HeadOffsetBytes == INVALID_OFFSET)
            {
                MM_DEBUG_EXPRESSION({
                    size_t localVar = TOCHARPTR(chunk) - TOCHARPTR(this);
                    MM_DEBUG_BREAK(localVar > 4'294'967'295);
                })
                m_HeadOffsetBytes = static_cast<uint32_t>(TOCHARPTR(chunk) - TOCHARPTR(this));
            }
            else
            {
                Chunk* currentHead = reinterpret_cast<Chunk*>(TOCHARPTR(this) + m_HeadOffsetBytes);
                m_HeadOffsetBytes = static_cast<uint32_t>(TOCHARPTR(chunk) - TOCHARPTR(this));
                MM_DEBUG_BREAK(m_HeadOffsetBytes > 4000000000)

                //  Before:            [current]-><-[other]-><-[other]
                //  After:  [freed]-><-[current]-><-[other]-><-[other]
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
                    // Remove this empty chunk from the free list (but keep at least one)
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

                    if (m_HeadOffsetBytes != INVALID_OFFSET)
                    {
                        Chunk* currentHead = reinterpret_cast<Chunk*>(TOCHARPTR(this) + m_HeadOffsetBytes);
                        if (chunk == currentHead)
                        {
                            if (next)
                            {
                                MM_DEBUG_EXPRESSION({
                                    size_t localVar = TOCHARPTR(next) - TOCHARPTR(this);
                                    MM_DEBUG_BREAK(localVar > 4'294'967'295);
                                })
                                m_HeadOffsetBytes = static_cast<uint32_t>(TOCHARPTR(next) - TOCHARPTR(this));
                            }
                            else
                                m_HeadOffsetBytes = INVALID_OFFSET;
                        }
                    }


                    return true;
                }
            }
            return false;
        }
    }

private:
    uint32_t m_HeadOffsetBytes;
    uint32_t m_NumChunks;
    const uint32_t m_SlotSize;
    const uint32_t m_SlotsCount;
};
#pragma endregion

#pragma region SLUBAllocator
struct SLUBAllocator
{
    SLUBAllocator(PageAllocator* pageAlloc, uint32_t pageSize, uint32_t minSlotSize)
        : m_PageSize(pageSize)
        , m_MinSlotSize(minSlotSize)
        , m_BasePool(nullptr)
        , m_PageAlloc(pageAlloc)
    {
        uint32_t poolsCount = CountSlots(CHUNK_PAYLOAD_SIZE(m_PageSize), 16);
        uint32_t offset = sizeof(Pool);

        uint32_t pages = 0;
        m_BasePool = pageAlloc->allocate_pages_size(poolsCount * offset, &pages);
        m_StaticOffset = m_PageSize * pages;

        for (size_t i = 0; i < poolsCount; i++)
        {
            uint32_t poolSize = static_cast<uint32_t>(m_MinSlotSize + (8 * i));
            unsigned char* poolPtr = m_BasePool + offset * i;
            new (poolPtr) Pool(poolSize, CHUNK_PAYLOAD_SIZE(m_PageSize) / poolSize);
        }
    }
    ~SLUBAllocator()
    {
        // TODO
    }

    const uint32_t m_PageSize;
    const uint32_t m_MinSlotSize;
    uint32_t m_StaticOffset;
    unsigned char* m_BasePool;

    PageAllocator* m_PageAlloc;

    //======================================================================
    template <class T, size_t Align = 0>
    [[nodiscard]] T* allocate(uint32_t slotSize, uint32_t count)
    {
        unsigned char* allocPtr = nullptr;
        Pool* pool = GetPool(slotSize);
        if (!pool)
            return nullptr;

        bool res = pool->AcquireSlotFromPool(allocPtr);
        if (!res)
        {
            uint32_t slotsPerPage = CHUNK_PAYLOAD_SIZE(m_PageSize) / slotSize;
            unsigned char* newPage = m_PageAlloc->allocate_page();
            if (newPage)
            {
                Chunk* newChunk = new (newPage) Chunk(slotSize, slotsPerPage);
                if (newChunk)
                {
                    MM_DEBUG_BREAK(!newChunk->IsValid());
                    pool->Extend(newChunk);
                    pool->AcquireSlotFromPool(allocPtr);
                }
            }
            else
            {
                // No space in PageAllocator
                MM_DEBUG_BREAK(true);
                return nullptr;
            }
        }
        return reinterpret_cast<T*>(allocPtr);
    }

    template <class T>
    void deallocate(void* ptr, uint32_t slotSize)
    {
        unsigned char* bytePtr = reinterpret_cast<unsigned char*>(ptr);

        auto pool = GetPool(slotSize);
        if (!pool)
            return;

        Chunk* chunk = FindChunk(bytePtr);
        MM_DEBUG_BREAK(!chunk->IsValid())
        bool isChunkEmpty = pool->FreeSlot(chunk, bytePtr);

        if (isChunkEmpty)
            m_PageAlloc->free(TOCHARPTR(chunk));
    }

    [[nodiscard]] Pool* GetPool(uint32_t slotSize)
    {
        size_t multiplier = slotSize / 8 - 2;
        return reinterpret_cast<Pool*>(m_BasePool + (sizeof(Pool) * multiplier));
    }
    [[nodiscard]] Chunk* FindChunk(unsigned char* ptrToSlot)
    {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptrToSlot);
        Chunk* chunk = reinterpret_cast<Chunk*>(addr / 4096 * 4096);

        MM_DEBUG_BREAK(!chunk->IsValid());
        return chunk;
    }

    void GetSnapshot(char*& buffer) const
    {
        buffer += snprintf(buffer, 1024l * 1024l * 50, "%s", "//-------------------------- SLUB ALLOCATOR ----------------------------\\\n");

        uint32_t poolsCount = CountSlots(CHUNK_PAYLOAD_SIZE(m_PageSize), 16);
        uint32_t offset = sizeof(Pool);

        for (size_t i = 0; i < poolsCount; i++)
        {
            Pool* pool = reinterpret_cast<Pool*>(m_BasePool + offset * i);
            pool->PoolSpapshot(buffer);
        }

        buffer += snprintf(buffer, 1024l * 1024l * 50, "\n//---------------------- END OF SLUB ALLOCATOR ----------------------------\\ \n\n");
    }

private:
    inline uint32_t CountSlots(uint32_t size, uint32_t slotSize = 8) const
    {
        uint32_t counter = 0;
        for (size_t i = size; i >= slotSize; i -= slotSize)
        {
            if (i / slotSize > 1)
                counter++;
        }

        return counter;
    }
};
#pragma endregion
