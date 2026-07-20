#include "FVkDepthTarget.h"

#include <cassert>

#include "VkHelper.h"

void FVkDepthTarget::Create(const FVkDevice* device, FVkCommandPool* immediateCommandPool, VkExtent2D extent, VkSampleCountFlagBits sampleCount,
                            bool sampled)
{
    Destroy();

    assert(device);
    assert(immediateCommandPool);

    m_DeviceContext = device;
    m_ImmediateCommandPoolContext = immediateCommandPool;
    m_Extent = extent;
    m_SampleCount = sampleCount;
    m_Sampled = sampled;
    m_Format = FindDepthFormat(m_DeviceContext->GetPhysicalDevice());

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = m_Format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | (sampled ? VK_IMAGE_USAGE_SAMPLED_BIT : 0);
    imageInfo.samples = sampleCount;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    m_Texture = std::make_unique<FVkTexture>();
    VkImage vkImage = m_Texture->CreateImage(m_DeviceContext->GetLogicalDevice(), m_DeviceContext->GetPhysicalDevice(), imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                              GetDepthAspect(m_Format));
    m_Texture->CreateImageView();

    {
        FVkSingleTimeCommandBuffer frameCmd = FVkSingleTimeCommandBuffer(m_DeviceContext->GetLogicalDevice(), m_ImmediateCommandPoolContext->GetCommandPool());
        frameCmd.TransitionImageLayout(vkImage, m_Format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                       GetDepthAspect(m_Format), 1, 1);
        frameCmd.Submit(m_DeviceContext->GetGraphicsQueue());
    }

    m_Initialized = true;
}

void FVkDepthTarget::Recreate(VkExtent2D extent, VkSampleCountFlagBits sampleCount, bool sampled)
{
    assert(m_DeviceContext);
    assert(m_ImmediateCommandPoolContext);
    Create(m_DeviceContext, m_ImmediateCommandPoolContext, extent, sampleCount, sampled);
}

void FVkDepthTarget::Destroy()
{
    m_Texture.reset();
    m_Format = VK_FORMAT_UNDEFINED;
    m_Extent = {0, 0};
    m_SampleCount = VK_SAMPLE_COUNT_1_BIT;
    m_Sampled = false;
    m_Initialized = false;
}

VkImageView FVkDepthTarget::GetImageView() const
{
    return m_Texture ? m_Texture->GetImageView() : VK_NULL_HANDLE;
}

VkImage FVkDepthTarget::GetImage() const
{
    return m_Texture ? m_Texture->GetImage() : VK_NULL_HANDLE;
}

VkFormat FVkDepthTarget::FindDepthFormat(VkPhysicalDevice device)
{
    return FindSupportedFormat(device, {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL,
                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
