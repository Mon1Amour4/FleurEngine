#include "FVkTexture.h"

#include <cassert>
#include <utility>

#include "VkHelper.h"

FVkTexture::FVkTexture()
    : m_Device(nullptr)
    , m_Format(VK_FORMAT_MAX_ENUM)
    , m_Aspect(VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM)
    , m_Image(nullptr)
    , m_ImageView(nullptr)
    , m_Memory(nullptr)
    , m_Mipmaps(0)
    , m_Layers(0)
{
}

FVkTexture::~FVkTexture()
{
    Destroy();
}

FVkTexture::FVkTexture(FVkTexture&& other) noexcept
{
    moveFrom(std::move(other));
}

FVkTexture& FVkTexture::operator=(FVkTexture&& other) noexcept
{
    if (this != &other)
    {
        Destroy();
        moveFrom(std::move(other));
    }

    return *this;
}

void FVkTexture::Destroy()
{
    if (m_Device)
    {
        vkDestroyImageView(m_Device, m_ImageView, nullptr);
        vkDestroyImage(m_Device, m_Image, nullptr);
        vkFreeMemory(m_Device, m_Memory, nullptr);
    }

    m_Device = nullptr;
    m_Format = VK_FORMAT_MAX_ENUM;
    m_Aspect = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
    m_Image = VK_NULL_HANDLE;
    m_ImageView = VK_NULL_HANDLE;
    m_Memory = VK_NULL_HANDLE;
    m_Mipmaps = 0;
    m_Layers = 0;
    m_ImageFlags = 0;
}

VkImage FVkTexture::CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, VkImageCreateInfo& createInfo, VkMemoryPropertyFlags properties,
                                VkImageAspectFlags aspect)
{
    Destroy();

    m_Device = device;
    m_Format = createInfo.format;
    m_Aspect = aspect;
    m_Mipmaps = createInfo.mipLevels;
    m_ImageFlags = createInfo.flags;
    m_Layers = createInfo.arrayLayers;

    VK_CHECK(vkCreateImage(m_Device, &createInfo, nullptr, &m_Image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_Device, m_Image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    VK_CHECK(vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_Memory));

    VK_CHECK(vkBindImageMemory(m_Device, m_Image, m_Memory, 0));

    return m_Image;
}

VkImageView FVkTexture::CreateImaveView()
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_Image;
    viewInfo.viewType = (m_ImageFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_Format;
    viewInfo.subresourceRange.aspectMask = m_Aspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_Mipmaps;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = m_Layers;

    VK_CHECK(vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageView));

    return m_ImageView;
}

void FVkTexture::moveFrom(FVkTexture&& other) noexcept
{
    m_Device = other.m_Device;
    m_Format = other.m_Format;
    m_Aspect = other.m_Aspect;
    m_Layers = other.m_Layers;
    m_Image = other.m_Image;
    m_ImageView = other.m_ImageView;
    m_Memory = other.m_Memory;
    m_Mipmaps = other.m_Mipmaps;
    m_ImageFlags = other.m_ImageFlags;

    other.m_Device = nullptr;
    other.m_Format = VK_FORMAT_MAX_ENUM;
    other.m_Aspect = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
    other.m_Layers = 0;
    other.m_Image = VK_NULL_HANDLE;
    other.m_ImageView = VK_NULL_HANDLE;
    other.m_Memory = VK_NULL_HANDLE;
    other.m_Mipmaps = 0;
    other.m_ImageFlags = 0;
}
