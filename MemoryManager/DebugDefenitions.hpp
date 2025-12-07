#pragma once

#pragma region MemoryManager Debug profiling definitions
//======================================================================
#if defined(_DEBUG) && defined(MEMORYMANAGER_PROFILING)
#include <algorithm>

struct MemoryInfo
{
    struct record
    {
        record(uint32_t slotSize)
            : allocAmount(0)
            , deallocAmount(0)
            , allocatedOverallMemoryBytes(0)
            , deallocatedOverallMemoryBytes(0) {};

        size_t allocAmount;
        size_t deallocAmount;
        size_t allocatedOverallMemoryBytes;
        size_t deallocatedOverallMemoryBytes;

        inline bool operator<(const record& other) const
        {
            return this->allocatedOverallMemoryBytes < other.allocatedOverallMemoryBytes;
        }
        inline bool operator>(const record& other) const
        {
            return this->allocatedOverallMemoryBytes > other.allocatedOverallMemoryBytes;
        }
        inline bool operator<=(const record& other)
        {
            return !(this->operator>(other));
        }
        inline bool operator>=(const record other)
        {
            return !(this->operator<(other));
        }
        inline bool operator==(const record& other)
        {
            return this->allocAmount == other.allocAmount;
        }
        inline bool operator!=(const record& other)
        {
            return !(this->operator==(other));
        }
    };

    MemoryInfo() = default;


    std::unordered_map<uint32_t, record> infos;

    void AddAlloc(size_t slotSize);
    void AddDealloc(size_t slotSize);
    static inline std::string FormatBytes(size_t bytes);

    void Print() const;
};

#define MM_ASSERT(expression) \
    do                        \
    {                         \
        assert(expression);   \
    } while (0);

#define MM_DEBUG_BREAK(expression) \
    do                             \
    {                              \
        if (expression == true)    \
        {                          \
            __debugbreak();        \
        }                          \
    } while (0);

#define MM_DEBUG_EXPRESSION(code) \
    do                            \
    {                             \
        code;                     \
    } while (0);

#define MM_PRINT(str)     \
    do                    \
    {                     \
        std::cout << str; \
    } while (0);


#else
#define MM_ASSERT(expression) ((void)0);
#define MM_DEBUG_BREAK(expression) ((void)0);
#define MM_DEBUG_EXPRESSION(code) ((void)0);
#define MM_PRINT(str) ((void)0);
#endif


#pragma endregion