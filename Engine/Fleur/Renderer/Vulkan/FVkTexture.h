#pragma once 

#include <vulkan/vulkan.h>

class FVkTexture
{
public:
    FVkTexture();
    ~FVkTexture();

    VkImage CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, VkImageCreateInfo createInfo, VkMemoryPropertyFlags properties,
                        VkImageAspectFlags aspect);
    VkImageView CreateImaveView();

    inline VkImageView GetImageView()
    {
        return m_ImageView;
    }
    inline VkImage GetImage()
    {
        return m_Image;
    }

private:
    VkDevice m_Device;
    VkFormat m_Format;
    VkImageAspectFlags m_Aspect;

    VkImage m_Image;
    VkImageView m_ImageView;
    VkDeviceMemory m_Memory;
};