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

    FVkDepthTarget(const FVkDepthTarget&) = delete;
    FVkDepthTarget& operator=(const FVkDepthTarget&) = delete;
    FVkDepthTarget(FVkDepthTarget&&) noexcept = default;
    FVkDepthTarget& operator=(FVkDepthTarget&&) noexcept = default;

    void Create(const FVkDevice* device, FVkCommandPool* immediateCommandPool, VkExtent2D extent, VkSampleCountFlagBits sampleCount,
                bool sampled = false, uint32_t layerCount = 1);
    void Recreate(VkExtent2D extent, VkSampleCountFlagBits sampleCount, bool sampled = false, uint32_t layerCount = 1);
    void Destroy();

    bool IsInitialized() const
    {
        return m_Initialized;
    }

    VkImageView GetImageView() const;
    VkImage GetImage() const;
    VkExtent2D GetExtent() const
    {
        return m_Extent;
    }
    VkFormat GetFormat() const
    {
        return m_Format;
    }

    static VkFormat FindDepthFormat(VkPhysicalDevice device);

private:
    // Non-owning context pointers. The caller must keep both objects alive until this target is destroyed.
    const FVkDevice* m_DeviceContext{nullptr};
    FVkCommandPool* m_ImmediateCommandPoolContext{nullptr};
    std::unique_ptr<FVkTexture> m_Texture;

    VkFormat m_Format{VK_FORMAT_UNDEFINED};
    VkExtent2D m_Extent{0, 0};
    VkSampleCountFlagBits m_SampleCount{VK_SAMPLE_COUNT_1_BIT};
    bool m_Sampled{false};
    uint32_t m_LayerCount{1};
    bool m_Initialized{false};
};
