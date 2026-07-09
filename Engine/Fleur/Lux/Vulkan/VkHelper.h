#pragma once

#include <vulkan/vulkan.h>

#include <stdexcept>
#include <string>
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

inline void VkCheck(VkResult result, const char* expr, const char* file, int line)
{
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(std::string("Vulkan call failed: ") + expr + " at " + file + ":" + std::to_string(line) +
                                 " VkResult=" + std::to_string(result));
    }
}

#define VK_CHECK(expr) VkCheck((expr), #expr, __FILE__, __LINE__)
