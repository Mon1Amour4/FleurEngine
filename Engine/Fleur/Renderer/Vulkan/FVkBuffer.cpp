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

FVkTexture::FVkTexture()
    : m_Device(nullptr)
    , m_Format(VK_FORMAT_MAX_ENUM)
    , m_Aspect(VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM)
    , m_Image(nullptr)
    , m_ImageView(nullptr)
    , m_Memory(nullptr)
{
}

FVkTexture::~FVkTexture()
{
    if (m_Device)
    {
        vkDestroyImage(m_Device, m_Image, nullptr);
        vkDestroyImageView(m_Device, m_ImageView, nullptr);
        vkFreeMemory(m_Device, m_Memory, nullptr);
    }
}

VkImage FVkTexture::CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, VkImageCreateInfo createInfo, VkMemoryPropertyFlags properties,
                                VkImageAspectFlags aspect)
{
    m_Device = device;
    m_Format = createInfo.format;
    m_Aspect = aspect;

    if (vkCreateImage(m_Device, &createInfo, nullptr, &m_Image) != VK_SUCCESS)
    {
        assert(true);
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_Device, m_Image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_Memory) != VK_SUCCESS)
    {
        assert(true);
    }

    vkBindImageMemory(m_Device, m_Image, m_Memory, 0);

    return m_Image;
}

VkImageView FVkTexture::CreateImaveView()
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_Image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_Format;
    viewInfo.subresourceRange.aspectMask = m_Aspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS)
    {
        assert(true);
    }

    return m_ImageView;
}
