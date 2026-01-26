#include "FVkBuffer.h"

#include <cassert>

#include "VkHelper.hpp"


FVkBuffer::FVkBuffer()
    : m_Device(nullptr)
    , m_Allocator(nullptr)
    , m_SizeBytes(0)
    , m_CurrentSizeBytes(0)
    , m_StrideSizeBytes(0)
    , m_VkBuffer(nullptr)
    , m_Allocation(nullptr)
    , m_MemoryUsage(0)
    , m_MappedMemory(nullptr)
{
}
FVkBuffer::~FVkBuffer()
{
    vmaDestroyBuffer(m_Allocator, m_VkBuffer, m_Allocation);
}

void FVkBuffer::Init(VmaAllocator allocator, VkDevice device, VkBufferUsageFlags usage, VkDeviceSize sizeBytes, VkDeviceSize strideSize)
{
    m_Allocator = allocator;
    m_Device = device;
    m_SizeBytes = sizeBytes;
    m_StrideSizeBytes = strideSize;

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeBytes;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &m_VkBuffer, &m_Allocation, nullptr) != VK_SUCCESS)
    {
        assert(false);
    }
}

void FVkBuffer::CopyToAnother(VkBuffer* dstBuffer, VkDeviceSize size, VkCommandBuffer* cmdBuffer)
{
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;  // Optional
    copyRegion.dstOffset = 0;  // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(*cmdBuffer, m_VkBuffer, *dstBuffer, 1, &copyRegion);
}

void FVkBuffer::MemCopy(const void* src, size_t size)
{
    vmaMapMemory(m_Allocator, m_Allocation, &m_MappedMemory);
    memcpy(m_MappedMemory, src, size);
    vmaUnmapMemory(m_Allocator, m_Allocation);
}

void FVkBuffer::UploadDataToBuffer(const void* pData, uint64_t count)
{
    uint64_t oldOffset = m_CurrentSizeBytes;
    m_CurrentSizeBytes += count * m_StrideSizeBytes;

    if (vmaCopyMemoryToAllocation(m_Allocator, pData, m_Allocation, oldOffset, m_StrideSizeBytes * count) != VK_SUCCESS)
    {
        assert(false);
    }
}

void* FVkBuffer::Map()
{
    vmaMapMemory(m_Allocator, m_Allocation, &m_MappedMemory);

    return m_MappedMemory;
}

void FVkBuffer::Unmap()
{
    if (!m_MappedMemory)
        return;

    vmaUnmapMemory(m_Allocator, m_Allocation);
    m_MappedMemory = nullptr;
}
