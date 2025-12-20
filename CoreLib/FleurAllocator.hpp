#pragma once
#include "MemoryManager.h"
#include "singleton.hpp"

namespace Fleur::Memory
{

class AllocAdapter : public Fleur::singleton<AllocAdapter>
{
    friend class singleton<AllocAdapter>;

    MM::MemoryManager* memory;

public:
    void Init(MM::MemoryManager* manager)
    {
        if (manager)
            memory = manager;
    }

    template <class T, size_t Align = 0>
    [[nodiscard]] T* allocate(uint32_t count = 1)
    {
        return memory->allocate<T, Align>(count);
    }

    template <class T>
    void deallocate(void* ptr, uint32_t count)
    {
        memory->deallocate<T>(ptr, count);
    }

    template <typename T, size_t Align = 0, typename... Args>
    [[nodiscard]] T* construct_at(Args&&... args)
    {
        return memory->construct_at<T, Align>(std::forward<Args>(args)...);
    }

    template <typename T, size_t Align = 0, typename... Args>
    [[nodiscard]] T* construct_array_at(uint32_t count, Args&&... args)
    {
        return memory->construct_array_at<T, Align>(count, std::forward<Args>(args)...);
    }
};

template <typename T>
class FleurAllocator
{
public:
    using value_type = T;

    FleurAllocator() = default;
    ~FleurAllocator() = default;

    [[nodiscard]] value_type* allocate(uint32_t count)
    {
        return AllocAdapter::instance().allocate<value_type>(count);
    }

    void deallocate(void* ptr, uint32_t count)
    {
        AllocAdapter::instance().deallocate<value_type>(ptr, count);
    }

    template <size_t Align = 0, typename... Args>
    [[nodiscard]] value_type* construct_at(Args... args)
    {
        return AllocAdapter::instance().construct_at<value_type>(std::forward<Args>(args)...);
    }
};
}  // namespace Fleur::Memory
