#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include <Graphics.hpp>

#include "FVkBuffer.h"
#include "FVkDevice.h"
#include "FVkPipeline.h"
#include "FVkShader.h"
#include "FVkSwapchain.h"

class FVkFloor
{
public:
    ~FVkFloor();
    void Create(const FVkDevice* device, const FVkSwapchain* swapchain, vk::FVkShader* shader, VkDescriptorSetLayout cameraLayout, VkImageView textureView,
                float height, VkSampleCountFlagBits sampleCount, VkFormat depthFormat);

    void SetFloor(VkImageView textureView);
    void SetFloor(VkImageView textureView, float height);

    void Record(VkCommandBuffer commandBuffer, VkDescriptorSet cameraDescriptor, VkExtent2D extent,
                const Fleur::Graphics::SFLCameraData& cameraData);

private:
    struct PushConstants
    {
        // x = grid height
        // y = half quad size
        // z = camera X
        // w = camera Z
        Fleur::Math::Vec4 gridParams;
    };

    VkDevice m_Device{VK_NULL_HANDLE};
    FVkPipeline* m_Pipeline{nullptr}; // borrowed from FVkShader::m_PipelineCache
    vk::FVkShader* m_Shader{nullptr};
    std::unique_ptr<FVkDescriptorSetLayout> m_TextureLayout;
    VkDescriptorPool m_DescriptorPool{VK_NULL_HANDLE};
    VkDescriptorSet m_DescriptorSet{VK_NULL_HANDLE};
    VkSampler m_Sampler{VK_NULL_HANDLE};
    float m_Height{0.0f};
    VkViewport m_Viewport{};
    VkRect2D m_Scissor{};
};
