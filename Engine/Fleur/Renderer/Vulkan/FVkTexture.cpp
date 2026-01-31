#include "FVkTexture.h"

#include <cassert>

#include "VkHelper.h"

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
    m_Mipmaps = createInfo.mipLevels;

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
    viewInfo.subresourceRange.levelCount = m_Mipmaps;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS)
    {
        assert(true);
    }

    return m_ImageView;
}