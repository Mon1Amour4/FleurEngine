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
                VkShaderModule fragmentShader,
                VkSampleCountFlagBits samples);
    // clang-format on

    void SetSkybox(VkImageView imageView);

    void Record(VkCommandBuffer cmd, uint32_t frame, VkFramebuffer frameBuffer, VkExtent2D swapchainExtent);

    void Update(uint32_t frame, const glm::mat4& transformation);

private:
    void CreateRenderPass(const FVkSwapchain* swapchain);
    void CreateDescriptorSetLayout();
    void CreateDescriptorPool(uint32_t framebufferCount);
    void CreateDescriptorSets(uint32_t framebufferCount);
    void CreateSampler();

    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;

    VkRenderPass m_RenderPass;
    VkShaderModule m_VertexShader;
    VkShaderModule m_FragmentShader;
    VkDescriptorSetLayout m_DescriptorSetLayout;
    VkCompareOp m_DepthStencilCompareOp;
    VkSampleCountFlagBits m_Samples;
    VkDescriptorPool m_DescriptorPool;
    std::vector<VkDescriptorSet> m_DescriptorSets;
    std::vector<FVkBuffer> m_UniformBuffers;
    VkSampler m_Sampler;

    VkImageView m_ImageView;

    FVkPipeline* m_Pipeline;
};
