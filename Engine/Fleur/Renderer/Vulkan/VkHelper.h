#pragma once

#include <vulkan/vulkan.h>

#include <vector>

uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void FindBarrierAccessMask(VkImageLayout oldLayout, VkImageLayout newLayout,
                           VkAccessFlags& srcAccessMask,  // VkImageMemoryBarrier
                           VkAccessFlags& dstAccessMask,  // VkImageMemoryBarrier
                           VkPipelineStageFlags& sourceStage, VkPipelineStageFlags& destinationStage);

uint32_t GetChannelsNumFromFormat(VkFormat format);
VkFormat GetVkFormat(uint32_t channels);
VkFormat FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
VkFormat FindDepthFormat(VkPhysicalDevice device);
VkImageAspectFlags GetDepthAspect(VkFormat format);
uint32_t CalculateMimMapLevel(uint32_t textureWidth, uint32_t textureHeight);

class SFLVertexInput
{
public:
    SFLVertexInput()
        : vertexStride(0) {};

    void RegisterAttribute(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset);

    std::vector<VkVertexInputAttributeDescription>& GetVertexDataAttributeDescriptions();

    VkVertexInputBindingDescription GetVertexDataBindingDescriptor();

private:
    uint32_t vertexStride;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
};
