#pragma once

#include <vulkan/vulkan.h>

#include <vector>

uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void FindBarrierAccessMask(VkImageLayout oldLayout, VkImageLayout newLayout,
                           VkAccessFlags& srcAccessMask,  // VkImageMemoryBarrier
                           VkAccessFlags& dstAccessMask,  // VkImageMemoryBarrier
                           VkPipelineStageFlags& sourceStage, VkPipelineStageFlags& destinationStage);

uint32_t GetChannelsNumFromFormat(VkFormat format);
VkFormat FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
VkFormat FindDepthFormat(VkPhysicalDevice device);
VkImageAspectFlags GetDepthAspect(VkFormat format);
uint32_t CalculateMimMapLevel(uint32_t textureWidth, uint32_t textureHeight);
bool HasStencilComponent(VkFormat format);

void transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask,
                           uint32_t mipMapCount);

uint32_t GetFormatSize(VkFormat format);

