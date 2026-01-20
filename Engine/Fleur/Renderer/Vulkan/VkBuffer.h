#pragma once

#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

class FVkBuffer
{
public:
    FVkBuffer(VmaAllocator allocator);
    FVkBuffer() = default;

    void Init(VkDevice device, VkBufferUsageFlags usage, VkDeviceSize sizeBytes, VkDeviceSize strideSize);

    void Allocate(VkDevice device, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags memoryUsage);

    void CopyTo(VkBuffer* dstBuffer, VkDeviceSize size, VkCommandBuffer* cmdBuffer);
    void UploadDataToBuffer(const void* pData, uint64_t count);

    inline uint64_t Size() const
    {
        return m_SizeBytes;
    }
    inline uint64_t CurrentSize() const
    {
        return m_CurrentSizeBytes;
    }
    inline uint32_t StrideBytes() const
    {
        return m_StrideSizeBytes;
    }
    inline VkBuffer& Buffer()
    {
        return m_VkBuffer;
    }
    inline VmaAllocation Allocation()
    {
        return m_Allocation;
    }
    inline VkDeviceMemory Memory() const
    {
        return m_VkMemory;
    }
    inline void* MappedMemory() const
    {
        return m_MappedMemory;
    }

    void* Map(VkDevice device);
    void Unmap(VkDevice device);

private:
    uint64_t m_SizeBytes;
    uint64_t m_CurrentSizeBytes;
    uint32_t m_StrideSizeBytes;

    VkBuffer m_VkBuffer;
    VkDeviceMemory m_VkMemory;

    VmaAllocation m_Allocation;
    VmaAllocator m_Allocator;

    VkMemoryPropertyFlags m_MemoryUsage;
    void* m_MappedMemory;
};