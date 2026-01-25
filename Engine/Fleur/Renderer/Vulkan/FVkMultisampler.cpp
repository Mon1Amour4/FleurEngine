#include "FVkMultisampler.h"

#include <cassert>

FVkMultisampler::FVkMultisampler()
    : m_MSAASamples(VK_SAMPLE_COUNT_1_BIT)
    , m_Device(nullptr)
{
}
FVkMultisampler::~FVkMultisampler()
{
}

void FVkMultisampler::Init(VkPhysicalDevice device, SFMultisamplerBuffersInfo* buffersInfo)
{
    m_Device = device;

    m_MSAASamples = getMaxUsableSampleCount(buffersInfo);
}

void FVkMultisampler::Enable(uint8_t level)
{
}

VkSampleCountFlagBits FVkMultisampler::getMaxUsableSampleCount(SFMultisamplerBuffersInfo* bufferFlags)
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(m_Device, &physicalDeviceProperties);

    VkSampleCountFlags counts = 0;
    if (bufferFlags->color && bufferFlags->depth)
        counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    else if (bufferFlags->color)
        counts = physicalDeviceProperties.limits.framebufferColorSampleCounts;
    else if (bufferFlags->depth)
        counts = physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    else
        assert(false);

    if (counts & VK_SAMPLE_COUNT_64_BIT)
    {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_32_BIT)
    {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_16_BIT)
    {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_8_BIT)
    {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_4_BIT)
    {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_2_BIT)
    {
        return VK_SAMPLE_COUNT_2_BIT;
    }

    return VK_SAMPLE_COUNT_1_BIT;
}
