#pragma once

#include <vulkan/vulkan.h>

#include "FVkTexture.h"

class FVkMultisampler
{
public:
    FVkMultisampler();
    ~FVkMultisampler();

    void Init(VkDevice device, VkPhysicalDevice physicalDevice, VkSampleCountFlagBits desiredSampleCount, uint32_t width, uint32_t height, VkFormat format);

    inline VkSampleCountFlagBits GetSamplesCount()
    {
        return m_MSAASamples;
    }
    inline FVkTexture* GetTexture()
    {
        return &m_Texture;
    }

private:
    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;
    VkSampleCountFlagBits m_MSAASamples;

    FVkTexture m_Texture;

    VkSampleCountFlagBits getMaxUsableSampleCount(bool depth);
};