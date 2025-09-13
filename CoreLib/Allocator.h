#pragma once

namespace Fleur::Core
{ /*
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
 };*/


template <typename T>
struct CustomAllocator
{
    using value_type = T;

    constexpr CustomAllocator() noexcept = default;
    constexpr ~CustomAllocator() = default;

    [[nodiscard]] constexpr T* allocate(size_t n) const
    {
        FL_CORE_INFO("[ALLOCATOR] Allocated {0} bytes for {1} of type", n * sizeof(T), n);
        return static_cast<T*>(malloc(n * sizeof(T)));
    }
    constexpr void deallocate(T* p, size_t n) const
    {
        FL_CORE_INFO("[ALLOCATOR] Deallocated {0} bytes for {1} of type", n * sizeof(T), n);
        free(p);
    }
};
}  // namespace Fleur::Core