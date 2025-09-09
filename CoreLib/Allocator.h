#pragma once

namespace Fleur::Core
{
static constexpr size_t PAGE_SIZE = 4096;

struct LinkedList
{
};

struct Slot
{
};

struct Chunk
{
private:
    constexpr size_t slotNum = 30;
};

template <typename T>
struct PoolAllocator
{
    PoolAllocator()
        : m_SlotsPerChunk(PAGE_SIZE / m_SlotSize)
        , m_ChunkNum(0) {};

    PoolAllocator(size_t slotsPerChunk)
        : m_SlotsPerChunk(slotsPerChunk)
        , m_ChunkNum(0) {};

    ~PoolAllocator()
    {
    }

    [[nodiscard]] void* allocate(size_t numBytes) noexcept;
    void deallocate(void* ptr, size_t nymBytes) noexcept;

private:
    constexpr size_t m_SlotSize = sizeof(T);
    constexpr size_t m_SlotsPerChunk;
    size_t m_ChunkNum;
};

}  // namespace Fleur::Core