#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <unordered_map>

enum class FVkAllocationCategory
{
    Buffer,
    Staging,
    Texture,
    RenderTarget,
};

class FVkMemoryTracker
{
public:
    void Init(VkPhysicalDevice physicalDevice, VkDevice device);

    VkDeviceMemory Allocate(const VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties, FVkAllocationCategory category);
    void Free(VkDeviceMemory memory);

    void PrintDiagnosis() const;
    size_t GetLiveAllocationCount() const;

private:
    struct AllocationInfo
    {
        VkDeviceSize size{};
        uint32_t memoryTypeIndex{};
        uint32_t heapIndex{};
        FVkAllocationCategory category{};
    };

    static uint64_t HandleKey(VkDeviceMemory memory);

    VkPhysicalDevice m_PhysicalDevice{VK_NULL_HANDLE};
    VkDevice m_Device{VK_NULL_HANDLE};
    VkPhysicalDeviceMemoryProperties m_MemoryProperties{};
    std::unordered_map<uint64_t, AllocationInfo> m_Allocations;
    std::array<VkDeviceSize, VK_MAX_MEMORY_HEAPS> m_TrackedBytesByHeap{};
    std::array<uint32_t, VK_MAX_MEMORY_HEAPS> m_TrackedAllocationCountByHeap{};
    std::array<VkDeviceSize, 5> m_TrackedBytesByCategory{};
    std::array<uint32_t, 5> m_TrackedAllocationCountByCategory{};
    VkDeviceSize m_TotalTrackedBytes{};
};
