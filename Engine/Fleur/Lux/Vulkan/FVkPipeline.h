#pragma once

#include <vulkan/vulkan.h>

#include <cassert>
#include <memory>

#include "VkHelper.h"

class FVkPipelineLayout;

struct FGraphicsPipelineDesc
{
    // Shaders
    VkShaderModule vertexShader;
    VkShaderModule fragmentShader;

    const std::vector<VkPushConstantRange>* pushConstants;

    // Vertex input
    const VkPipelineVertexInputStateCreateInfo* pVertexInputState;

    // IA
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Raster / Depth
    bool depthTestEnable = false;
    bool depthWriteEnable = true;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;

    VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
    VkFrontFace frontFace = VK_FRONT_FACE_MAX_ENUM;

    // Depth bias
    bool depthBiasEnable = false;
    float depthBiasConstantFactor = 0.0f;
    float depthBiasClamp = 0.0f;
    float depthBiasSlopeFactor = 0.0f;

    // Formats (dynamic rendering)
    uint32_t colorAttachmentCount = 1;
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    // MSAA
    VkSampleCountFlagBits samplesCount = VK_SAMPLE_COUNT_1_BIT;

    const char* vertexEntryPointName = nullptr;
    const char* fragmentEntryPointName = nullptr;

    const std::vector<VkPipelineShaderStageCreateInfo>* shaderStages = nullptr;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = false,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
};

class FVkPipeline
{
public:
    FVkPipeline();
    ~FVkPipeline();

    void Init(VkDevice device, FGraphicsPipelineDesc& desc, const std::shared_ptr<FVkPipelineLayout>& pipelineLayout);
    void Destroy();

    VkPipeline GetPipeline() const
    {
        return m_Pipeline;
    }
    VkPipelineLayout GetPipelineLayout() const
    {
        return m_PipelineLayout;
    }
private:
    VkDevice m_Device;
    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;
    std::shared_ptr<FVkPipelineLayout> m_LayoutOwner;

};
