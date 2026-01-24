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

private: 
    VkPhysicalDevice m_Device;
    VkSampleCountFlagBits m_MSAASamples;

    VkSampleCountFlagBits getMaxUsableSampleCount(SFMultisamplerBuffersInfo* bufferFlags);
};