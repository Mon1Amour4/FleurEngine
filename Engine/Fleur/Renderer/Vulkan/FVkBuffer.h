#pragma once

#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

class FVkBuffer
{
public:
    FVkBuffer();
    ~FVkBuffer();

    void Init(VmaAllocator allocator, VkDevice device, VkBufferUsageFlags usage, VkDeviceSize sizeBytes, VkDeviceSize strideSize);

    void CopyToAnother(VkBuffer* dstBuffer, VkDeviceSize size, VkCommandBuffer* cmdBuffer);
    void MemCopy(const void* src, size_t size);
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

    inline void* MappedMemory() const
    {
        return m_MappedMemory;
    }

    void* Map();
    void Unmap();

private:
    VmaAllocator m_Allocator;
    VkDevice m_Device;

    uint64_t m_SizeBytes;
    uint64_t m_CurrentSizeBytes;
    uint32_t m_StrideSizeBytes;

    VkBuffer m_VkBuffer;
    // VkDeviceMemory m_VkMemory;

    VmaAllocation m_Allocation;

    VkMemoryPropertyFlags m_MemoryUsage;
    void* m_MappedMemory;
};
