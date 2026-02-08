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

    void Create(VkDevice device, VkExtent2D extent, VkShaderModule vertexShader, VkShaderModule fragmentShader);

    inline FVkTexture* GetCubemapTexture()
    {
        return &m_Texture;
    }

private:
    void CreateRenderPass(const FVkSwapchain* swapchain);

    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;

    VkRenderPass m_RenderPass;
    VkShaderModule m_VertexShader;
    VkShaderModule m_FragmentShader;

    FVkTexture* m_Texture;
    FVkPipeline* m_Pipeline;
    SFLVertexInput* m_VertexInput;
};
