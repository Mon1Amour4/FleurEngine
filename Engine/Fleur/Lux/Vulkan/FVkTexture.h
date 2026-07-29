#pragma once

#include <vulkan/vulkan.h>

class FVkTexture
{
public:
    FVkTexture();
    ~FVkTexture();

    FVkTexture(const FVkTexture&) = delete;
    FVkTexture& operator=(const FVkTexture&) = delete;
    FVkTexture(FVkTexture&& other) noexcept;
    FVkTexture& operator=(FVkTexture&& other) noexcept;

    VkImage CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, VkImageCreateInfo& createInfo, VkMemoryPropertyFlags properties,
                        VkImageAspectFlags aspect);
    VkImageView CreateImageView();
    VkImageView CreateImageView(VkImageViewType viewType, uint32_t layerCount);

    void Destroy();

    inline VkImageView GetImageView() const
    {
        return m_ImageView;
    }
    inline VkImage GetImage() const
    {
        return m_Image;
    }

private:
    VkDevice m_Device;
    VkFormat m_Format;
    VkImageType m_ImageType;
    VkImageAspectFlags m_Aspect;
    uint32_t m_Layers;

    VkImage m_Image;
    VkImageView m_ImageView;
    VkDeviceMemory m_Memory;

    uint32_t m_Mipmaps;

    VkImageCreateFlags m_ImageFlags;

    void moveFrom(FVkTexture&& other) noexcept;
};
