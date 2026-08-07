#include "FVkTexture.h"

#include <cassert>
#include <utility>

#include "VkHelper.h"

FVkTexture::FVkTexture()
    : m_Device(nullptr)
    , m_MemoryTracker(nullptr)
    , m_Format(VK_FORMAT_MAX_ENUM)
    , m_ImageType(VK_IMAGE_TYPE_MAX_ENUM)
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
        assert(m_MemoryTracker != nullptr);
        m_MemoryTracker->Free(m_Memory);
    }

    m_Device = nullptr;
    m_MemoryTracker = nullptr;
    m_Format = VK_FORMAT_MAX_ENUM;
    m_ImageType = VK_IMAGE_TYPE_MAX_ENUM;
    m_Aspect = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
    m_Image = VK_NULL_HANDLE;
    m_ImageView = VK_NULL_HANDLE;
    m_Memory = VK_NULL_HANDLE;
    m_Mipmaps = 0;
    m_Layers = 0;
    m_ImageFlags = 0;
}

VkImage FVkTexture::CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, FVkMemoryTracker& memoryTracker, FVkAllocationCategory category,
                                VkImageCreateInfo& createInfo, VkMemoryPropertyFlags properties, VkImageAspectFlags aspect)
{
    (void)physicalDevice;
    Destroy();

    m_Device = device;
    m_MemoryTracker = &memoryTracker;
    m_Format = createInfo.format;
    m_ImageType = createInfo.imageType;
    m_Aspect = aspect;
    m_Mipmaps = createInfo.mipLevels;
    m_ImageFlags = createInfo.flags;
    m_Layers = createInfo.arrayLayers;

    VK_CHECK(vkCreateImage(m_Device, &createInfo, nullptr, &m_Image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_Device, m_Image, &memRequirements);

    m_Memory = m_MemoryTracker->Allocate(memRequirements, properties, category);
    if (m_Memory == VK_NULL_HANDLE)
    {
        vkDestroyImage(m_Device, m_Image, nullptr);
        m_Image = VK_NULL_HANDLE;
        return VK_NULL_HANDLE;
    }

    const VkResult bindResult = vkBindImageMemory(m_Device, m_Image, m_Memory, 0);
    if (bindResult != VK_SUCCESS)
    {
        m_MemoryTracker->Free(m_Memory);
        m_Memory = VK_NULL_HANDLE;
        vkDestroyImage(m_Device, m_Image, nullptr);
        m_Image = VK_NULL_HANDLE;
        VK_CHECK(bindResult);
    }

    return m_Image;
}

VkImageView FVkTexture::CreateImageView()
{
    const VkImageViewType viewType = (m_ImageFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
                                         ? VK_IMAGE_VIEW_TYPE_CUBE
                                         : (m_ImageType == VK_IMAGE_TYPE_3D ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D);
    return CreateImageView(viewType, m_Layers);
}

VkImageView FVkTexture::CreateImageView(VkImageViewType viewType, uint32_t layerCount)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_Image;
    viewInfo.viewType = viewType;
    viewInfo.format = m_Format;
    viewInfo.subresourceRange.aspectMask = m_Aspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_Mipmaps;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = layerCount;

    VK_CHECK(vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageView));

    return m_ImageView;
}

void FVkTexture::moveFrom(FVkTexture&& other) noexcept
{
    m_Device = other.m_Device;
    m_MemoryTracker = other.m_MemoryTracker;
    m_Format = other.m_Format;
    m_ImageType = other.m_ImageType;
    m_Aspect = other.m_Aspect;
    m_Layers = other.m_Layers;
    m_Image = other.m_Image;
    m_ImageView = other.m_ImageView;
    m_Memory = other.m_Memory;
    m_Mipmaps = other.m_Mipmaps;
    m_ImageFlags = other.m_ImageFlags;

    other.m_Device = nullptr;
    other.m_MemoryTracker = nullptr;
    other.m_Format = VK_FORMAT_MAX_ENUM;
    other.m_ImageType = VK_IMAGE_TYPE_MAX_ENUM;
    other.m_Aspect = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
    other.m_Layers = 0;
    other.m_Image = VK_NULL_HANDLE;
    other.m_ImageView = VK_NULL_HANDLE;
    other.m_Memory = VK_NULL_HANDLE;
    other.m_Mipmaps = 0;
    other.m_ImageFlags = 0;
}
