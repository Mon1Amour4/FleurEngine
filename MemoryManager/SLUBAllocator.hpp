#pragma once
#include "DebugDefenitions.hpp"
#include "PageAllocator.hpp"

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
        , m_NextChunk(nullptr)
        , m_PrevChunk(nullptr)
    {
        LOCAL_HEAD(this, Chunk);

        MM_DEBUG_BREAK(m_CapacityBytes > 4096);
        MM_PRINT("--[CREATED]\n")
        MM_PRINT("  Chunk{" << this << "}{" << m_SlotSize << ", " << slotCount << "} has been created")

        MM_PRINT(", Head{" << static_cast<void*>(localHead) << "}, Tail{" << static_cast<void*>(localHead + m_CapacityBytes) << "}\n")

        // TODO A place for optimization
        for (size_t i = 0; i < slotCount; i++)
        {
            uint32_t* slotPtr = reinterpret_cast<uint32_t*>(localHead + (m_SlotSize * i));
            if (i == slotCount - 1)
            {
                *slotPtr = II_NULL_INDEX;
                MM_DEBUG_BREAK(!IsValid());
            }
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

            MM_PRINT("--[Slot Aquired] 0x" << static_cast<void*>(outPtr) << ", Chunk{0x" << static_cast<void*>(this) << "}, " << m_SlotSize << "/"
                                           << m_CapacityBytes / m_SlotSize << "} {" << ", Old capacity: " << oldCapacity << ", Used Bytes: " << m_UsedBytes
                                           << std::endl;);

            uint32_t nextIdx = *reinterpret_cast<uint32_t*>(outPtr);
            if (nextIdx == II_NULL_INDEX)
            {
                // Slot was last free slot in this chunk
                m_FreeSlot = II_NULL_INDEX;
                // MM_DEBUG_BREAK(m_FreeSlot == II_NULL_INDEX && m_UsedBytes < m_CapacityBytes)
                MM_DEBUG_BREAK(!IsValid());
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
        uint32_t deallocatedIdx = (ptrToSLot - localHead) / m_SlotSize;
        if (wasFull)
        {
            MM_DEBUG_BREAK(deallocatedIdx == II_NULL_INDEX)

            m_FreeSlot = deallocatedIdx;
            uint32_t* slot = reinterpret_cast<uint32_t*>(localHead + (m_FreeSlot * m_SlotSize));
            *slot = II_NULL_INDEX;
            MM_DEBUG_BREAK(!IsValid());
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

        MM_PRINT("--[Slot Free] " << "0x" << static_cast<void*>(ptrToSLot) << ", Used Bytes: " << m_UsedBytes << std::endl;)
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

        Chunk* nextChunk = GetNext();
        buffer += std::sprintf(buffer, " |%-3d| 0x%-16p |%4d/%-4d|%4d/%-d|%-7s|\tnext{0x%-16p} |\n", chunkNum, this, m_SlotSize, m_CapacityBytes / m_SlotSize,
                               m_UsedBytes, m_CapacityBytes, sign.c_str(), nextChunk);
    }

    void SetNext(Chunk* next)
    {
        if (next == this)
            return;

        if (!next)
        {
            m_NextChunk = nullptr;
            return;
        }
        m_NextChunk = next;
    }
    void SetPrev(Chunk* prev)
    {
        if (prev == this)
            return;

        if (!prev)
        {
            m_PrevChunk = nullptr;
            return;
        }
        m_PrevChunk = prev;
    }
    inline Chunk* GetNext()
    {
        if (!m_NextChunk)
            return nullptr;

        return m_NextChunk;
    }
    inline Chunk* GetPrev()
    {
        if (!m_PrevChunk)
            return nullptr;

        return m_PrevChunk;
    }

    inline bool IsValid() const
    {
        return m_UsedBytes < 5000;
    }

private:
    const uint32_t m_SlotSize;
    const uint32_t m_CapacityBytes;

    uint32_t m_UsedBytes;

    uint32_t m_FreeSlot;

    // Offset: n * (sizeof(Chunk) + PageSize) bytes
    Chunk* m_NextChunk;
    Chunk* m_PrevChunk;

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
        : m_NumChunks(0)
        , m_SlotSize(slotSize)
        , m_SlotsCount(slotCount)
        ,  // m_HeadOffsetBytes(INVALID_OFFSET)
        m_Head(nullptr)
    {
        MM_ASSERT(m_SlotSize > 0);
        MM_ASSERT(m_SlotsCount > 0);

        MM_PRINT("--[CREATED]\n");
        MM_PRINT("  Pool{" << std::to_string(m_SlotSize) << ", " << std::to_string(static_cast<uint32_t>(m_SlotsCount)) << "} has been created\n ")
    }
    ~Pool()
    {
        if (m_Head)
            m_Head->~Chunk();
    }

    // False - no space
    bool AcquireSlotFromPool(unsigned char*& outPtr)
    {
        if (!m_Head)
            return false;

        if (m_Head->AcquireSlot(outPtr))
            return true;
        else
        {
            // We've acquired the last free slot, now we need to get next free chunk and set it as Head
            Chunk* nextHead = m_Head->GetNext();
            m_Head->SetNext(nullptr);
            m_Head->SetPrev(nullptr);
            if (nextHead)
            {
                nextHead->SetPrev(nullptr);
                MM_DEBUG_EXPRESSION({
                    size_t localVar = TOCHARPTR(nextHead) - TOCHARPTR(this);
                    MM_DEBUG_BREAK(localVar > 4'294'967'295);
                })
                m_Head = nextHead;
            }
            else
                m_Head = nullptr;

            return outPtr != nullptr;
        }
    }

    void Extend(Chunk* chunk)
    {
        MM_PRINT("Pool{" << this << "} extend by chunk{" << &*chunk << "}\n");

        if (m_Head == nullptr)
        {
            MM_DEBUG_EXPRESSION({
                size_t localVar = TOCHARPTR(chunk) - TOCHARPTR(this);
                MM_DEBUG_BREAK(localVar > 4'294'967'295);
            })
            m_Head = chunk;
        }
        else
        {
            Chunk* currentHead = m_Head;
            m_Head = chunk;
            currentHead->SetPrev(chunk);
            chunk->SetNext(currentHead);
        }
        m_NumChunks++;
    }

    void Print()
    {
        std::cout << "\nPrinting Pool{" << m_SlotSize << "," << m_SlotsCount << "}: Chunks: " << std::to_string(m_NumChunks);

        m_Head->Print(0);
    }
    void PoolSpapshotToStream(std::ofstream& stream)
    {
        stream << "\nPrinting Pool{" << this << "}{" << m_SlotSize << ", " << m_SlotsCount << "} : Chunks: " << std::to_string(m_NumChunks);
        m_Head->ChunkSnapshotToStream(0, stream);
    }
    void PoolSpapshot(char*& buffer)
    {
        if (m_Head)
        {
            MM_DEBUG_BREAK(!m_Head->IsValid())
            buffer +=
                std::sprintf(buffer, "\nPrinting Pool{%p}{%d, %d}, Chunks: %d, free chunk: 0x%-18p\n", this, m_SlotSize, m_SlotsCount, m_NumChunks, m_Head);
            m_Head->ChunkSnapshot(0, buffer);
        }
    }

    // True - Chunk is empty
    bool FreeSlot(Chunk* chunk, unsigned char* ptrToSlot)
    {
        bool isChunkEmpty = false;
        if (chunk->FreeChunkSlot(ptrToSlot, &isChunkEmpty))
        {
            // We need to add this chunk to a free list

            if (!m_Head)
            {
                MM_DEBUG_EXPRESSION({
                    size_t localVar = TOCHARPTR(chunk) - TOCHARPTR(this);
                    MM_DEBUG_BREAK(localVar > 4'294'967'295);
                })
                m_Head = chunk;
            }
            else
            {
                Chunk* currentHead = m_Head;
                m_Head = chunk;

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

                    if (m_Head)
                    {
                        Chunk* currentHead = m_Head;
                        if (chunk == currentHead)
                        {
                            if (next)
                            {
                                MM_DEBUG_EXPRESSION({
                                    size_t localVar = TOCHARPTR(next) - TOCHARPTR(this);
                                    MM_DEBUG_BREAK(localVar > 4'294'967'295);
                                })
                                m_Head = next;
                            }
                            else
                                m_Head = nullptr;
                        }
                    }


                    return true;
                }
            }
            return false;
        }
    }

private:
    // uint32_t m_HeadOffsetBytes;
    Chunk* m_Head;
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
        , m_ChunkCache(nullptr)
        , m_PageAlloc(pageAlloc)
        , m_BasePool(nullptr)
    {
        uint32_t poolsCount = CountSlots(CHUNK_PAYLOAD_SIZE(m_PageSize), 16);
        uint32_t offset = sizeof(Pool);

        uint32_t pages = 0;
        m_BasePool = pageAlloc->allocate_pages_size(poolsCount * offset, &pages);
        m_StaticOffset = m_PageSize * pages;

        for (size_t i = 0; i < poolsCount; i++)
        {
            uint32_t poolSize = m_MinSlotSize + (8 * i);
            unsigned char* poolPtr = m_BasePool + offset * i;
            Pool* pool = new (poolPtr) Pool(poolSize, CHUNK_PAYLOAD_SIZE(m_PageSize) / poolSize);
        }
    }
    ~SLUBAllocator()
    {
        // TODO
    }

    const uint32_t m_PageSize;
    const uint32_t m_MinSlotSize;
    uint32_t m_StaticOffset;
    void* m_ChunkCache;
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

        // Check first if there is Cached Chunk in free list:
        if (m_ChunkCache)
        {
            void* next = *reinterpret_cast<void**>(m_ChunkCache);
            unsigned char* cachedChunk = TOCHARPTR(m_ChunkCache);
            Chunk* chunk = new (cachedChunk) Chunk(slotSize, m_PageSize / slotSize);
            pool->Extend(chunk);

            m_ChunkCache = next;
        }
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

        if (allocPtr)
        {
            if constexpr (std::is_trivially_constructible_v<T>)
            {
                return reinterpret_cast<T*>(allocPtr);
            }

            // placement new
            if (count > 1)
            {
                // Array
                T* arrayPtr = reinterpret_cast<T*>(allocPtr);

                for (size_t i = 0; i < count; i++)
                {
                    new (arrayPtr + i) T;
                }

                return arrayPtr;
            }
            else
                return new (allocPtr) T;
        }
        else
        {
            // No space in Arena for new chunk
            MM_DEBUG_BREAK(true);
            return nullptr;
        }
    }

    template <class T, size_t Align = 0>
    [[nodiscard]] T* allocate_raw(uint32_t count)
    {
        // TODO
    }

    template <class T>
    void deallocate(void* ptr, uint32_t slotSize, uint32_t count)
    {
        unsigned char* bytePtr = reinterpret_cast<unsigned char*>(ptr);

        auto pool = GetPool(slotSize);
        if (!pool)
            return;

        Chunk* chunk = FindChunk(bytePtr);
        bool isChunkEmpty = pool->FreeSlot(chunk, bytePtr);

        if (isChunkEmpty)
        {
            // Check if this chunk is latest allocated chunk
            LOCAL_HEAD(this, SLUBAllocator);
            if (TOCHARPTR(chunk) == (localHead - (sizeof(Chunk) + m_PageSize)))
            {
                // This is latest allocated chunk, we don't want to store it
                m_PageAlloc->free_latest_allocated_page();
            }
            else
            {
                if (!m_ChunkCache)
                {
                    m_ChunkCache = reinterpret_cast<void*>(chunk);
                    *reinterpret_cast<void**>(m_ChunkCache) = nullptr;
                }
                else
                {
                    void* current = m_ChunkCache;
                    m_ChunkCache = reinterpret_cast<void*>(chunk);
                    *reinterpret_cast<void**>(chunk) = current;
                }
            }
        }

        if constexpr (std::is_trivially_destructible_v<T>)
        {
            return;
        }
        // placement new deallocation
        if (count > 1)
        {
            T* arrayPtr = reinterpret_cast<T*>(bytePtr);
            for (size_t i = 0; i < count; i++)
            {
                (arrayPtr + i)->~T();
            }
        }
        else
        {
            reinterpret_cast<T*>(bytePtr)->~T();
        }
    }

    template <class T>
    void free(void* ptr, uint32_t count)
    {
        // TODO
    }

    [[nodiscard]] Pool* GetPool(uint32_t slotSize)
    {
        size_t multiplier = slotSize / 8 - 2;
        return reinterpret_cast<Pool*>(m_BasePool + (sizeof(Pool) * multiplier));
    }
    [[nodiscard]] Chunk* FindChunk(unsigned char* ptrToSlot)
    {
        unsigned char* contentHead = m_BasePool + m_StaticOffset;
        MM_DEBUG_BREAK(ptrToSlot < contentHead);

        size_t diff = ptrToSlot - contentHead;
        size_t pageIndex = diff / m_PageSize;


        unsigned char* page = contentHead + pageIndex * m_PageSize;
        Chunk* chunk = reinterpret_cast<Chunk*>(page);

        MM_DEBUG_BREAK(!chunk->IsValid());
        return chunk;
    }

private:
    inline uint32_t CountSlots(uint32_t size, uint32_t slotSize = 8)
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