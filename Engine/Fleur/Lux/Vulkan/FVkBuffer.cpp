#include "FVkBuffer.h"

#include <cassert>

#include "VkHelper.h"


FVkBuffer::FVkBuffer()
    : m_Device(nullptr)
    , m_SizeBytes(0)
    , m_CurrentSizeBytes(0)
    , m_StrideSizeBytes(0)
    , m_VkBuffer(nullptr)
    , m_MemoryUsage(0)
    , m_MappedMemory(nullptr)
{
}
FVkBuffer::~FVkBuffer()
{
    if (m_MappedMemory)
    {
        vkUnmapMemory(m_Device, m_Memory);
        m_MappedMemory = nullptr;
    }

    if (m_VkBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_Device, m_VkBuffer, nullptr);
        m_VkBuffer = VK_NULL_HANDLE;
    }

    if (m_Memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_Device, m_Memory, nullptr);
        m_Memory = VK_NULL_HANDLE;
    }
}

void FVkBuffer::Init(VkDevice device, VkPhysicalDevice physicalDevice, VkBufferUsageFlags usage, VkDeviceSize sizeBytes, VkDeviceSize strideSize)
{
    m_Device = device;
    m_SizeBytes = sizeBytes;
    m_StrideSizeBytes = strideSize;

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeBytes;
    bufferInfo.usage = usage;

    VK_CHECK(vkCreateBuffer(m_Device, &bufferInfo, nullptr, &m_VkBuffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_Device, m_VkBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VK_CHECK(vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_Memory));

    VK_CHECK(vkBindBufferMemory(m_Device, m_VkBuffer, m_Memory, 0));
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
    vkMapMemory(m_Device, m_Memory, 0, size, 0, &m_MappedMemory);
    memcpy(m_MappedMemory, src, size);
    vkUnmapMemory(m_Device, m_Memory);
    m_MappedMemory = nullptr;
}

void FVkBuffer::UploadDataToBuffer(const void* pData, uint64_t count)
{
    const VkDeviceSize sizeBytes = count * m_StrideSizeBytes;
    const VkDeviceSize offset = m_CurrentSizeBytes;

    m_CurrentSizeBytes += sizeBytes;

    assert(offset + sizeBytes <= m_SizeBytes);

    vkMapMemory(m_Device, m_Memory, offset, sizeBytes, 0, &m_MappedMemory);
    memcpy(m_MappedMemory, pData, sizeBytes);
    vkUnmapMemory(m_Device, m_Memory);
    m_MappedMemory = nullptr;
}

void* FVkBuffer::Map()
{
    vkMapMemory(m_Device, m_Memory, 0, m_SizeBytes, 0, &m_MappedMemory);

    return m_MappedMemory;
}

void FVkBuffer::Unmap()
{
    if (!m_MappedMemory)
        return;

    vkUnmapMemory(m_Device, m_Memory);
    m_MappedMemory = nullptr;
}
