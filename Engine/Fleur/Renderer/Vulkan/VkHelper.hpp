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