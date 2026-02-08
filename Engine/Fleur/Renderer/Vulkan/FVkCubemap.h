#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "FVkDevice.h"
#include "FVkPipeline.h"
#include "FVkSwapchain.h"
#include "FVkTexture.h"

class FVkSkybox
{
public:
    FVkSkybox();
    ~FVkSkybox();

    void Create(const FVkDevice* device, const FVkSwapchain* swapchain, VkShaderModule vertexShader, VkShaderModule fragmentShader);
    inline FVkTexture* GetCubemapTexture()
    {
        return &m_Texture;
    }

private:
    void CreateRenderPass(const FVkSwapchain* swapchain);
    void CreateDescriptorSetLayout();
    void CreateDescriptorPool(uint32_t framebufferCount);
    void CreateDescriptorSets(uint32_t framebufferCount);

    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;

    VkRenderPass m_RenderPass;
    VkShaderModule m_VertexShader;
    VkShaderModule m_FragmentShader;
    VkDescriptorSetLayout m_DescriptorSetLayout;
    VkCompareOp m_DepthStencilCompareOp;
    VkDescriptorPool m_DescriptorPool;
    std::vector<VkDescriptorSet> m_DescriptorSets;
    std::vector<FVkBuffer> m_UniformBuffers;

    FVkTexture* m_Texture;
    FVkPipeline* m_Pipeline;
    SFLVertexInput* m_VertexInput;
};
