#include "FVkBuffer.h"

#include <cassert>

#include "VkHelper.h"


FVkBuffer::FVkBuffer()
    : m_Device(nullptr)
    , m_MemoryTracker(nullptr)
    , m_SizeBytes(0)
    , m_CurrentSizeBytes(0)
    , m_StrideSizeBytes(0)
    , m_VkBuffer(nullptr)
    , m_Memory(VK_NULL_HANDLE)
    , m_MemoryUsage(0)
    , m_MappedMemory(nullptr)
{
}

FVkBuffer::FVkBuffer(FVkBuffer&& other) noexcept
{
    moveFrom(std::move(other));
}

FVkBuffer& FVkBuffer::operator=(FVkBuffer&& other) noexcept
{
    if (this != &other)
    {
        Destroy();
        moveFrom(std::move(other));
    }

    return *this;
}

FVkBuffer::~FVkBuffer()
{
    Destroy();
}

void FVkBuffer::Destroy()
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
        assert(m_MemoryTracker != nullptr);
        m_MemoryTracker->Free(m_Memory);
        m_Memory = VK_NULL_HANDLE;
    }

    m_Device = VK_NULL_HANDLE;
    m_MemoryTracker = nullptr;
    m_SizeBytes = 0;
    m_CurrentSizeBytes = 0;
    m_StrideSizeBytes = 0;
    m_MemoryUsage = 0;
}

void FVkBuffer::Init(VkDevice device, VkPhysicalDevice physicalDevice, FVkMemoryTracker& memoryTracker, FVkAllocationCategory category,
                     VkBufferUsageFlags usage, VkDeviceSize sizeBytes, VkDeviceSize strideSize)
{
    (void)physicalDevice;
    Destroy();
    m_Device = device;
    m_MemoryTracker = &memoryTracker;
    m_SizeBytes = sizeBytes;
    m_StrideSizeBytes = strideSize;

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeBytes;
    bufferInfo.usage = usage;

    VK_CHECK(vkCreateBuffer(m_Device, &bufferInfo, nullptr, &m_VkBuffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_Device, m_VkBuffer, &memRequirements);

    m_Memory = m_MemoryTracker->Allocate(memRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, category);
    if (m_Memory == VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_Device, m_VkBuffer, nullptr);
        m_VkBuffer = VK_NULL_HANDLE;
        return;
    }

    const VkResult bindResult = vkBindBufferMemory(m_Device, m_VkBuffer, m_Memory, 0);
    if (bindResult != VK_SUCCESS)
    {
        m_MemoryTracker->Free(m_Memory);
        m_Memory = VK_NULL_HANDLE;
        vkDestroyBuffer(m_Device, m_VkBuffer, nullptr);
        m_VkBuffer = VK_NULL_HANDLE;
        VK_CHECK(bindResult);
    }
}

void FVkBuffer::moveFrom(FVkBuffer&& other) noexcept
{
    m_Device = other.m_Device;
    m_MemoryTracker = other.m_MemoryTracker;
    m_SizeBytes = other.m_SizeBytes;
    m_CurrentSizeBytes = other.m_CurrentSizeBytes;
    m_StrideSizeBytes = other.m_StrideSizeBytes;
    m_VkBuffer = other.m_VkBuffer;
    m_Memory = other.m_Memory;
    m_MemoryUsage = other.m_MemoryUsage;
    m_MappedMemory = other.m_MappedMemory;

    other.m_Device = VK_NULL_HANDLE;
    other.m_MemoryTracker = nullptr;
    other.m_VkBuffer = VK_NULL_HANDLE;
    other.m_Memory = VK_NULL_HANDLE;
    other.m_MappedMemory = nullptr;
    other.m_SizeBytes = 0;
    other.m_CurrentSizeBytes = 0;
    other.m_StrideSizeBytes = 0;
    other.m_MemoryUsage = 0;
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
