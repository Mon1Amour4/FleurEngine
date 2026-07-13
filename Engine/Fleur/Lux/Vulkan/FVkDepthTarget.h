#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include "FVkCommand.h"
#include "FVkDevice.h"
#include "FVkTexture.h"

class FVkDepthTarget
{
public:
    FVkDepthTarget() = default;
    ~FVkDepthTarget() = default;

    void Create(const FVkDevice* device, FVkCommandPool* immediateCommandPool, VkExtent2D extent, VkSampleCountFlagBits sampleCount,
                bool sampled = false);
    void Recreate(VkExtent2D extent, VkSampleCountFlagBits sampleCount, bool sampled = false);
    void Destroy();

    bool IsInitialized() const
    {
        return m_Initialized;
    }

    VkImageView GetImageView() const;
    VkImage GetImage() const;
    VkFormat GetFormat() const
    {
        return m_Format;
    }

    static VkFormat FindDepthFormat(VkPhysicalDevice device);

private:
    const FVkDevice* m_Device{nullptr};
    FVkCommandPool* m_ImmediateCommandPool{nullptr};
    std::unique_ptr<FVkTexture> m_Texture;

    VkFormat m_Format{VK_FORMAT_UNDEFINED};
    VkExtent2D m_Extent{0, 0};
    VkSampleCountFlagBits m_SampleCount{VK_SAMPLE_COUNT_1_BIT};
    bool m_Sampled{false};
    bool m_Initialized{false};
};
