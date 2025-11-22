#pragma once

struct PageAllocator
{
    PageAllocator(unsigned char* ptr, size_t capacity, uint32_t pageSize)
        : m_Current(ptr)
        , m_CachedPage(nullptr)
        , m_Capacity(capacity)
        , m_UsedBytes(0)
        , m_PageSize(pageSize) {};

    unsigned char* allocate_page()
    {
        if (m_UsedBytes + m_PageSize > m_Capacity)
            return nullptr;

        unsigned char* pagePtr = m_Current;

        m_UsedBytes += m_PageSize;
        m_Current += m_PageSize;

        return pagePtr;
    }
    unsigned char* allocate_pages_n(uint32_t pagesCount)
    {
        size_t bytes = pagesCount * m_PageSize;
        if (m_UsedBytes + bytes > m_Capacity)
            return nullptr;

        unsigned char* pagePtr = m_Current;

        m_UsedBytes += bytes;
        m_Current += bytes;

        return pagePtr;
    }
    unsigned char* allocate_pages_size(size_t bytes, uint32_t* pagesCount)
    {
        uint32_t pages = bytes / m_PageSize;
        uint32_t reminder = bytes % m_PageSize;
        if (reminder > 0)
            pages++;

        size_t pagesSizeBytes = m_PageSize * pages;
        if (m_UsedBytes + pagesSizeBytes > m_Capacity)
            return nullptr;

        unsigned char* pagePtr = m_Current;

        m_UsedBytes += pagesSizeBytes;
        m_Current += pagesSizeBytes;

        *pagesCount = pages;
        return pagePtr;
    }

    void free(unsigned char* ptrToPage)
    {
        if (m_UsedBytes < m_PageSize)
            return;

        if (ptrToPage == m_Current - m_PageSize)
        {
            // Front Page, just move ptr back
            m_Current -= m_PageSize;
            m_UsedBytes -= m_PageSize;
        }
        else
        { /*if (!m_ChunkCache)
         //     {
         //         m_ChunkCache = reinterpret_cast<Chunk*>(chunk);
         //         MM_DEBUG_BREAK(!m_ChunkCache->IsValid());
         //         *reinterpret_cast<Chunk**>(m_ChunkCache) = nullptr;
         //     }*/
            //     else
            //     {

            //        // Chunk* current = m_ChunkCache;
            //        // MM_DEBUG_BREAK(!current->IsValid())
            //        // m_ChunkCache = reinterpret_cast<Chunk*>(chunk);
            //        //*reinterpret_cast<Chunk**>(chunk) = current;
            //    }
        }
    }

    unsigned char* m_Current;
    unsigned char* m_CachedPage;
    size_t m_Capacity;
    size_t m_UsedBytes;
    uint32_t m_PageSize;

private:
    unsigned char* check_cache()
    {
        // Check first if there is Cached Chunk in free list:
        /* if (m_ChunkCache)
         {
             Chunk* next = *reinterpret_cast<Chunk**>(m_ChunkCache);
             unsigned char* cachedChunk = TOCHARPTR(m_ChunkCache);

             Chunk* chunk = new (cachedChunk) Chunk(slotSize, m_PageSize / slotSize);

             MM_DEBUG_BREAK(!chunk->IsValid());

             pool->Extend(chunk);

             m_ChunkCache = next;
         }*/
    }
};