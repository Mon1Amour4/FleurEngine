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