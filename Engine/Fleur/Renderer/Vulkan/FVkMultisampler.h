#pragma once

#include <vulkan/vulkan.h>

struct SFMultisamplerBuffersInfo
{
    bool color;
    bool depth;
};

class FVkMultisampler
{
public:
    FVkMultisampler();
    ~FVkMultisampler();

    void Init(VkPhysicalDevice device, SFMultisamplerBuffersInfo* buffersInfo);

    void Enable(uint8_t level);

    inline VkImage* GetImage()
    {
        return &m_ColorImage;
    }
    inline VkDeviceMemory* GetImageMemory()
    {
        return &m_ColorImageMemory;
    }
    inline VkImageView* GetImageView()
    {
        return &m_ColorImageView;
    }

private:
    VkPhysicalDevice m_Device;
    VkSampleCountFlagBits m_MSAASamples;

    VkImage m_ColorImage;
    VkDeviceMemory m_ColorImageMemory;
    VkImageView m_ColorImageView;

    VkSampleCountFlagBits getMaxUsableSampleCount(SFMultisamplerBuffersInfo* bufferFlags);
};