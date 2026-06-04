#pragma once
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#pragma region MemoryManager Debug profiling definitions
//======================================================================
#if defined(_DEBUG) && defined(MEMORYMANAGER_PROFILING)


struct MemoryInfo
{
    struct record
    {
        record() = default;

        size_t allocAmount{0};
        size_t deallocAmount{0};
        size_t allocatedOverallMemoryBytes{0};
        size_t deallocatedOverallMemoryBytes{0};

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
        if ((expression) == true)  \
        {                          \
            __debugbreak();        \
        }                          \
    } while (0);

#define MM_DEBUG_EXPRESSION(code) \
    do                            \
    {                             \
        code;                     \
    } while (0);

#define MM_PRINT(str) \
    //do                    \
    //{                     \
    //    std::cout << str; \
    //} while (0);


#else
#define MM_ASSERT(expression) ((void)0);
#define MM_DEBUG_BREAK(expression) ((void)0);
#define MM_DEBUG_EXPRESSION(code) ((void)0);
#define MM_PRINT(str) ((void)0);
#endif

// Release-ACTIVE contract check (unlike MM_DEBUG_BREAK / MM_ASSERT, which are
// debug-only invariants compiled out in release). A failure means unrecoverable
// corruption or a broken build-time contract, so fail loud instead of continuing
// into silent UB. Use ONLY on cold paths (init, fatal-corruption detection) —
// never the hot allocate/free path. Recoverable conditions (OOM) must NOT use
// this; they return nullptr instead.
#define MM_VERIFY(expr)                                                                            \
    do                                                                                             \
    {                                                                                              \
        if (!(expr))                                                                               \
        {                                                                                          \
            std::cerr << "MM_VERIFY failed: " #expr " (" << __FILE__ << ":" << __LINE__ << ")\n";  \
            std::abort();                                                                          \
        }                                                                                          \
    } while (0)

#define II_NULL_INDEX 0xffffffff
#define INVALID_OFFSET 0xFFFFFFFF
#define LOCAL_HEAD(ptr, type) unsigned char* localHead = reinterpret_cast<unsigned char*>(ptr) + sizeof(type)
#define TOCHARPTR(ptr) reinterpret_cast<unsigned char*>(ptr)
#define CHUNK_STRIDE (sizeof(Chunk) + 4096)

#pragma endregion