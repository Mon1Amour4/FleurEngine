#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

#include "FVkTexture.h"

class ShadowMapOffsetTexture
{
public:
    ShadowMapOffsetTexture() = default;
    ~ShadowMapOffsetTexture();

    ShadowMapOffsetTexture(const ShadowMapOffsetTexture&) = delete;
    ShadowMapOffsetTexture& operator=(const ShadowMapOffsetTexture&) = delete;

    void Generate(VkExtent2D extent, uint32_t filterSize);
    void Create(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, VkExtent2D extent, uint32_t filterSize);
    void Destroy();

    const std::vector<float>& GetData() const { return m_Data; }
    VkExtent2D GetExtent() const { return m_Extent; }
    VkExtent3D GetTextureExtent() const { return m_TextureExtent; }
    uint32_t GetFilterSize() const { return m_FilterSize; }
    VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
    VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }
    VkImageView GetImageView() const { return m_Texture.GetImageView(); }

private:
    static float jitter();

    VkDevice m_Device{VK_NULL_HANDLE};
    VkDescriptorPool m_DescriptorPool{VK_NULL_HANDLE};
    VkDescriptorSetLayout m_DescriptorSetLayout{VK_NULL_HANDLE};
    VkDescriptorSet m_DescriptorSet{VK_NULL_HANDLE};
    VkSampler m_Sampler{VK_NULL_HANDLE};
    FVkTexture m_Texture;

    VkExtent2D m_Extent{0, 0};
    VkExtent3D m_TextureExtent{0, 0, 0};
    uint32_t m_FilterSize{0};
    std::vector<float> m_Data;
};
