#pragma once

#include <vulkan/vulkan.h>

#include <vector>

static uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    assert(false);
}

// clang-format off
static void FindBarrierAccessMask(  VkImageLayout oldLayout, 
                                    VkImageLayout newLayout, 
                                    VkAccessFlags& srcAccessMask, // VkImageMemoryBarrier
                                    VkAccessFlags& dstAccessMask, // VkImageMemoryBarrier
                                    VkPipelineStageFlags& sourceStage, 
                                    VkPipelineStageFlags& destinationStage)
{
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        srcAccessMask = 0;
        dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        srcAccessMask = 0;
        dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        assert(false);
    }
}
// clang-format onn

static uint32_t GetChannelsNumFromFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_R8G8B8A8_UNORM:
        return 4;
    case VK_FORMAT_R8_UNORM:
        return 1;
    default:
        assert(false);
        break;
    }
}

static VkFormat GetVkFormat(uint32_t channels)
{
    VkFormat format{};
        switch (channels)
        {
        case 1:
            format = VK_FORMAT_R8_UNORM;
            break;
        case 3:
            format = VK_FORMAT_R8G8B8A8_SRGB;
            break;
        case 4:
            format = VK_FORMAT_R8G8B8A8_SRGB;
            break;
        }
        return format;
}

static VkFormat FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                                               VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }

        assert(false);
    }
}
static VkFormat FindDepthFormat(VkPhysicalDevice device)
{
    VkFormat format = FindSupportedFormat(device, {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL,
                                          VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    return format;
}

static VkImageAspectFlags GetDepthAspect(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D16_UNORM:
            return VK_IMAGE_ASPECT_DEPTH_BIT;

        default:
            assert(false && "Invalid depth format");
            return 0;
    }
}

class SFLVertexInput
{
public:
    SFLVertexInput()
        : vertexStride(0) {};

    void RegisterAttribute(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset)
    {
        attributeDescriptions.emplace_back(location, binding, format, offset);
        vertexStride += offset;
    }

    std::vector<VkVertexInputAttributeDescription>& GetVertexDataAttributeDescriptions()
    {
        return attributeDescriptions;
    }

    VkVertexInputBindingDescription GetVertexDataBindingDescriptor()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = vertexStride;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

private:
    uint32_t vertexStride;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
};