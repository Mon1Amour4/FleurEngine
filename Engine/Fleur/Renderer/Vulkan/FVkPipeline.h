#pragma once

#include <vulkan/vulkan.h>
#include "VkHelper.hpp"

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
};

class FVkPipeline
{
public:
    FVkPipeline();
    ~FVkPipeline();

    void Init(SFPipelineCreationInfo* info);

    VkPipeline Pipeline()
    {
        return m_Pipeline;
    }
    VkPipelineLayout PipelineLayout()
    {
        return m_PipelineLayout;
    }

private:
    VkDevice m_Device;
    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;

};