#pragma once

#include <vulkan/vulkan.h>

#include "VkHelper.h"
enum BindingType
{
    UNIFORM_BUFFER,
    TEXTURE2D,
    CUBEMAP,
};
struct SFPipelineCreationInfo
{
    VkDevice device;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkShaderModule vertexShader;
    VkShaderModule fragmentShader;
    uint32_t pushConstantSize;
    SFLVertexInput* vertexInput;
    VkPrimitiveTopology topology;
    VkViewport* viewport;
    VkExtent2D extent;
    VkSampleCountFlagBits samplesCount;
    VkCompareOp depthStencilOp;
    VkBool32 depthWriteEnable;
    BindingType binding;
};

class FVkPipeline
{
public:
    FVkPipeline();
    ~FVkPipeline();

    void Init(SFPipelineCreationInfo* info);

    VkPipeline GetPipeline()
    {
        return m_Pipeline;
    }
    VkPipelineLayout GetPipelineLayout()
    {
        return m_PipelineLayout;
    }

private:
    VkDevice m_Device;
    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;

    uint32_t GetBindingIdx();
};