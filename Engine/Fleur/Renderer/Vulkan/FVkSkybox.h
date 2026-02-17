#pragma once

#include <vulkan/vulkan.h>

#include <RenderViews.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <vector>

#include "FVkCommand.h"
#include "FVkDevice.h"
#include "FVkPipeline.h"
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
                VkShaderModule vertexShader,
                VkShaderModule fragmentShader);
    // clang-format on

    void SetSkybox(VkImageView imageView);

    void Record(VkCommandBuffer& cmd, VkExtent2D swapchainExtent, Fleur::Graphics::SFLCameraData& cameraData);

private:
    void createDescriptorSetLayout();
    void createPipeline();
    void createDescriptorPool();
    void createSampler();
    void createDescriptorSet(VkImageView imageView);

    void updateDescriptorSet(VkImageView imageView);

    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;
    FVkDescriptorSetLayout* m_DescriptorSetLayout;
    SFLVertexInput* m_VertexInput;
    FVkPipeline* m_Pipeline;

    VkSampler m_Sampler;
    VkDescriptorPool m_DescriptorPool;
    VkDescriptorSet m_DescriptorSet;

    VkShaderModule m_VertexShader;
    VkShaderModule m_FragmentShader;

    VkFormat m_ColorFormat;
    VkExtent2D m_Extent;

    VkViewport m_DefaultViewport;
    VkRect2D m_DefaultRect;

    FVkBuffer* m_VertexBuffer;
    const uint32_t m_SizeOfUniformBuffer;
    FVkBuffer* m_UniformBuffer;

    static const uint32_t m_VertexCount = 36;
    static const uint32_t m_VertexBufferSize = m_VertexCount * sizeof(glm::vec3);

    static const glm::vec3 m_Vertices[m_VertexCount];
};