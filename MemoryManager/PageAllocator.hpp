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

        unsigned char* pagePtr = check_cache();
        if (pagePtr)
            return pagePtr;

        pagePtr = m_Current;
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
        uint32_t pages = (uint32_t)(bytes / m_PageSize);
        uint32_t reminder = bytes % m_PageSize;
        if (reminder > 0)
            pages++;

        if (pages == 1)
            return allocate_page();

        size_t pagesSizeBytes = m_PageSize * pages;
        if (m_UsedBytes + pagesSizeBytes > m_Capacity)
            return nullptr;

        unsigned char* pagePtr = m_Current;

        m_UsedBytes += pagesSizeBytes;
        m_Current += pagesSizeBytes;

        if (pagesCount)
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
        {
            if (m_CachedPage)
            {
                unsigned char* current = m_CachedPage;
                m_CachedPage = reinterpret_cast<unsigned char*>(ptrToPage);
                *reinterpret_cast<unsigned char**>(m_CachedPage) = current;
                return;
            }

            m_CachedPage = reinterpret_cast<unsigned char*>(ptrToPage);
            *reinterpret_cast<unsigned char**>(m_CachedPage) = nullptr;
        }
    }

    unsigned char* m_Current;
    unsigned char* m_CachedPage;
    size_t m_Capacity;
    size_t m_UsedBytes;
    uint32_t m_PageSize;

    void GetSnapshot(char*& buffer) const
    {
        if (!m_CachedPage)
            return;

        buffer += sprintf_s(buffer, 1024l * 1024l * 50, "%s", "\n//---------------------------- PAGE ALLOCATOR ----------------------------\\\n");

        buffer += sprintf_s(buffer, 1024l * 1024l * 50, "{%zu/%zu}\n", m_UsedBytes, m_Capacity);
        buffer += sprintf_s(buffer, 1024l * 1024l * 50, "Cached page:{0x%p}, ", static_cast<void*>(m_CachedPage));
        unsigned char* next = *reinterpret_cast<unsigned char**>(m_CachedPage);
        while (next)
        {
            buffer += sprintf_s(buffer, 1024l * 1024l * 50, "{0x%p}", static_cast<void*>(next));
            next = *reinterpret_cast<unsigned char**>(next);
        }
        buffer += sprintf_s(buffer, 1024l * 1024l * 50, "\n//------------------------ END OF PAGE ALLOCATOR ------------------------------\\\n\n");
    }

private:
    inline unsigned char* check_cache()
    {
        // Check first if there is Cached page in free list:
        if (m_CachedPage)
        {
            unsigned char* next = *reinterpret_cast<unsigned char**>(m_CachedPage);
            unsigned char* cachedChunk = m_CachedPage;

            m_CachedPage = next;

            return cachedChunk;
        }

        return nullptr;
    }
};