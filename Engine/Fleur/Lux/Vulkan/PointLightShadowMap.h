#pragma once

#include <vulkan/vulkan.h>

#include <Fleur/Math/Types.hpp>
#include <array>
#include <cstdint>
#include <vector>

#include "DescriptorPoolAllocator.h"
#include "FVkBuffer.h"
#include "FVkCommand.h"
#include "FVkPipeline.h"
#include "FVkPipelineCache.h"
#include "FVkPipelineLayout.h"
#include "FVkShader.h"
#include "FVkTexture.h"

class PointLightShadowMap
{
public:
    struct PushConstant
    {
        uint32_t modelIdx{0};
        uint32_t nodeIdx{0};
    };

    explicit PointLightShadowMap(uint32_t textureCount);
    ~PointLightShadowMap();

    PointLightShadowMap(const PointLightShadowMap&) = delete;
    PointLightShadowMap& operator=(const PointLightShadowMap&) = delete;

    void Create(VkDevice device, VkPhysicalDevice physicalDevice, FVkMemoryTracker& memoryTracker, uint32_t resolution, VkFormat depthFormat);
    void CreatePipeline(vk::FVkShader& shader);
    void Begin(FVkCommandBuffer& commandBuffer, uint32_t lightIndex);
    void End(FVkCommandBuffer& commandBuffer);
    void PrepareForSampling(FVkCommandBuffer& commandBuffer);
    PushConstant MakePushConstant(uint32_t modelIdx, uint32_t nodeIdx) const
    {
        return {modelIdx, nodeIdx};
    }
    void UpdateMatrices(uint32_t lightIndex, const std::array<Fleur::Mat4, 6>& matrices);
    void Destroy();

    uint32_t GetTextureCount() const
    {
        return m_TextureCount;
    }
    VkPipeline GetPipeline() const
    {
        return m_Pipeline != nullptr ? m_Pipeline->GetPipeline() : VK_NULL_HANDLE;
    }
    VkPipelineLayout GetPipelineLayout() const
    {
        return m_Pipeline != nullptr ? m_Pipeline->GetPipelineLayout() : VK_NULL_HANDLE;
    }
    VkImageView GetArrayImageView(uint32_t index) const
    {
        return m_ArrayImageViews.at(index);
    }
    VkImageView GetCubeImageView(uint32_t index) const
    {
        return m_PointLightShadowMaps.at(index).GetImageView();
    }
    FVkTexture& GetTexture(uint32_t index)
    {
        return m_PointLightShadowMaps.at(index);
    }
    const FVkTexture& GetTexture(uint32_t index) const
    {
        return m_PointLightShadowMaps.at(index);
    }

private:
    struct ShadowMatrices
    {
        Fleur::Mat4 viewProjection[6];
    };

    void createDescriptorSet();
    void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

    uint32_t m_TextureCount{0};
    VkDevice m_Device{VK_NULL_HANDLE};
    VkPhysicalDevice m_PhysicalDevice{VK_NULL_HANDLE};
    VkExtent2D m_Extent{0, 0};
    VkFormat m_DepthFormat{VK_FORMAT_UNDEFINED};

    std::vector<FVkTexture> m_PointLightShadowMaps;
    std::vector<VkImageView> m_ArrayImageViews;
    std::vector<VkDescriptorSet> m_MatricesDescriptorSets;
    bool m_ImagesInitialized{false};

    FVkBuffer m_MatricesBuffer;

    uint32_t m_ActiveLightIndex{UINT32_MAX};
    vk::abstraction::DescriptorAllocator m_DescriptorAllocator;

    FVkPipeline* m_Pipeline{nullptr};
    std::shared_ptr<FVkPipelineLayout> m_PipelineLayout;
    FVkPipelineCache m_PipelineCache;
};
