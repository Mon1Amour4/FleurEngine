#include <cassert>

#include "VkBuffer.h"
#include "VkHelper.hpp"


FVkBuffer::FVkBuffer(VmaAllocator allocator)
    : m_SizeBytes(0)
    , m_CurrentSizeBytes(0)
    , m_StrideSizeBytes(0)
    , m_VkBuffer(nullptr)
    , m_VkMemory(nullptr)
    , m_Allocation(nullptr)
    , m_Allocator(allocator)
    , m_MemoryUsage(0)
    , m_MappedMemory(nullptr)
{
}

void FVkBuffer::Init(VkDevice device, VkBufferUsageFlags usage, VkDeviceSize sizeBytes, VkDeviceSize strideSize)
{
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
void FVkBuffer::Allocate(VkDevice device, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags memoryUsage)
{
    m_MemoryUsage = memoryUsage;

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, m_VkBuffer, &memRequirements);

    uint32_t memoryType = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, memoryUsage);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryType;

    if (vkAllocateMemory(device, &allocInfo, nullptr, &m_VkMemory) != VK_SUCCESS)
    {
        // DBG_PRINTM("failed to allocate buffer memory!!")
        assert(false);
    }

    vkBindBufferMemory(device, m_VkBuffer, m_VkMemory, 0);
}

void FVkBuffer::CopyTo(VkBuffer* dstBuffer, VkDeviceSize size, VkCommandBuffer* cmdBuffer)
{
    //VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;  // Optional
    copyRegion.dstOffset = 0;  // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(*cmdBuffer, m_VkBuffer, *dstBuffer, 1, &copyRegion);

    //endSingleTimeCommands(commandBuffer);
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

void* FVkBuffer::Map(VkDevice device)
{
    assert(m_MemoryUsage && VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT != 0);

    vkMapMemory(device, m_VkMemory, 0, m_SizeBytes, 0, &m_MappedMemory);

    return m_MappedMemory;
}

void FVkBuffer::Unmap(VkDevice device)
{
    if (!m_MappedMemory)
        return;

    vkUnmapMemory(device, m_VkMemory);
}
