#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include <RenderViews.hpp>
#include <Fleur/Math/Math.hpp>
#include <vector>

#include "FVkCommand.h"
#include "FVkDevice.h"
#include "FVkPipeline.h"
#include "FVkPipelineCache.h"
#include "FVkPipelineLayout.h"
#include "FVkShader.h"
#include "FVkSwapchain.h"
#include "FVkTexture.h"
#include "Graphics.hpp"

class FVkSkybox
{
public:
    FVkSkybox();
    ~FVkSkybox();

    // clang-format off
    void Create(const FVkDevice* device, 
                const FVkSwapchain* swapchain, 
                VkImageView imageView, 
                vk::FVkShader* skyboxShader,
                VkSampleCountFlagBits sampleCount,
                VkFormat depthFormat);
    // clang-format on

    void SetSkybox(VkImageView imageView);

    void Record(VkCommandBuffer& cmd, VkExtent2D swapchainExtent, const Fleur::Graphics::SFLCameraData& cameraData);

private:
    void createSkyboxDescriptorPool();
    void createSkyboxSampler();
    void createSkyboxDescriptorSet(VkImageView imageView);

    void updateSkyboxDescriptorSet(VkImageView imageView);

    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;
    FVkPipeline* m_Pipeline{nullptr}; // borrowed from m_PipelineCache
    std::shared_ptr<FVkPipelineLayout> m_PipelineLayout;
    FVkPipelineCache m_PipelineCache;

    VkSampler m_Sampler;
    VkDescriptorPool m_SkyboxDescriptorPool;
    VkDescriptorSet m_SkyboxDescriptorSet;

    VkSampleCountFlagBits m_SampleCount;

    vk::FVkShader* m_SkyboxShader;

    VkFormat m_ColorFormat;
    VkFormat m_DepthFormat;

    VkExtent2D m_Extent;

    VkViewport m_DefaultViewport;
    VkRect2D m_DefaultRect;

    std::unique_ptr<FVkBuffer> m_VertexBuffer;
    const uint32_t m_SizeOfUniformBuffer;
    std::unique_ptr<FVkBuffer> m_UniformBuffer;

    static const uint32_t m_VertexCount = 36;
    static const uint32_t m_VertexBufferSize = m_VertexCount * sizeof(Fleur::Vec3);

    static const Fleur::Vec3 m_Vertices[m_VertexCount];
};
