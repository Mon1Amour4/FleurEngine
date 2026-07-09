#pragma once

#include <spirv_reflect.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

#include <functional>
#include <map>
#include <unordered_map>

#include "FVkPipeline.h"

namespace vk
{
struct ShaderCreateInfo
{
    const void* const pVertexData;
    size_t vertexSize;

    const void* const pFragmentData;
    size_t fragmentSize;
};

struct GetPipelineInfo
{
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
    VkSampleCountFlagBits samplesCount = VK_SAMPLE_COUNT_1_BIT;

    VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
    VkFrontFace frontFace = VK_FRONT_FACE_MAX_ENUM;

    bool depthTestEnable = false;
    bool depthWriteEnable = true;

    // Depth bias
    bool depthBiasEnable = false;
    float depthBiasConstantFactor = 0.0f;
    float depthBiasClamp = 0.0f;
    float depthBiasSlopeFactor = 0.0f;

    // Dynamic rendering formats
    uint32_t colorAttachmentCount = 1;
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    bool blendEnable = false;

    inline bool operator==(const GetPipelineInfo& rhs) const noexcept
    {
        return topology == rhs.topology && depthCompareOp == rhs.depthCompareOp && samplesCount == rhs.samplesCount && cullMode == rhs.cullMode &&
               frontFace == rhs.frontFace && depthTestEnable == rhs.depthTestEnable && depthWriteEnable == rhs.depthWriteEnable &&

               depthBiasEnable == rhs.depthBiasEnable && depthBiasConstantFactor == rhs.depthBiasConstantFactor && depthBiasClamp == rhs.depthBiasClamp &&
               depthBiasSlopeFactor == rhs.depthBiasSlopeFactor &&

               colorAttachmentCount == rhs.colorAttachmentCount && colorFormat == rhs.colorFormat && depthFormat == rhs.depthFormat &&

               blendEnable == rhs.blendEnable;
    }
};
}  // namespace vk


namespace std
{
template <>
struct hash<vk::GetPipelineInfo>
{
    size_t operator()(const vk::GetPipelineInfo& info) const noexcept
    {
        size_t h = 0;

        auto hashCombine = [&h](size_t value) { h ^= value + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2); };

        hashCombine(std::hash<uint32_t>{}(static_cast<uint32_t>(info.topology)));
        hashCombine(std::hash<uint32_t>{}(static_cast<uint32_t>(info.depthCompareOp)));
        hashCombine(std::hash<uint32_t>{}(static_cast<uint32_t>(info.samplesCount)));
        hashCombine(std::hash<uint32_t>{}(info.cullMode));
        hashCombine(std::hash<uint32_t>{}(static_cast<uint32_t>(info.frontFace)));
        hashCombine(std::hash<bool>{}(info.depthTestEnable));
        hashCombine(std::hash<bool>{}(info.depthWriteEnable));
        hashCombine(std::hash<uint32_t>{}(info.colorFormat));
        hashCombine(std::hash<uint32_t>{}(info.depthFormat));
        hashCombine(std::hash<bool>{}(info.blendEnable));

        return h;
    }
};
}  // namespace std


namespace vk
{
class FVkShader
{
public:
    struct ShaderData
    {
        ShaderData()
            : shaderModule(nullptr)
            , entryPoint()
            , shaderStage(VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM) {};

        VkShaderModule shaderModule;


        // ---------- VkPipelineShaderStageCreateInfo ----------
        std::string entryPoint;
        VkShaderStageFlagBits shaderStage;
    };
    FVkShader();
    ~FVkShader();

    void Init(VkDevice device, ShaderCreateInfo& info);

    inline VkShaderModule GetVertexShader() const
    {
        return m_VertexShader.shaderModule;
    }
    inline VkShaderModule GetFragmentShader() const
    {
        return m_FragmentShader.shaderModule;
    }

    inline const VkPipelineVertexInputStateCreateInfo* GetVertexInputState() const
    {
        return &m_VertexInput.createInfo;
    }

    inline const std::vector<VkPushConstantRange>& GetPushConstants() const
    {
        return m_PushConstants;
    }

    inline const FVkShader::ShaderData& GetVertexShaderData() const
    {
        return m_VertexShader;
    }
    inline const FVkShader::ShaderData& GetFragmentShaderData() const
    {
        return m_FragmentShader;
    }

    FVkPipeline* GetPipeline(const GetPipelineInfo& info, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    struct DescriptorSetDefaultValues
    {
        uint32_t sampler2d = 4096;
    };

    bool isInitialized() const
    {
        return m_VertexShader.shaderModule && m_FragmentShader.shaderModule;
    }

private:
    VkDevice m_Device;

    ShaderData m_VertexShader;
    ShaderData m_FragmentShader;

    struct VertexInput
    {
        std::vector<VkVertexInputAttributeDescription> m_VertexInputAttributes;
        VkVertexInputBindingDescription bindingDescription;
        VkPipelineVertexInputStateCreateInfo createInfo;
    } m_VertexInput;


    std::vector<VkPushConstantRange> m_PushConstants;
    std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> m_GlobalDescriptorSetLayoutBindingsMap;

    void getReflection(ShaderData& shaderData, const void* const pVertexData, size_t vertexSize);
    void mergePushConstants();
    VkShaderStageFlagBits convertReflectionShaderStage(SpvReflectShaderStageFlagBits stage);
    VkDescriptorType convertReflectionDescriptorType(SpvReflectDescriptorType type);
    VkFormat convertReflectionFormat(SpvReflectFormat format);

    // ---------- pipeline ----------
    std::unordered_map<GetPipelineInfo, FVkPipeline> m_PipelineCache;

    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;


    static DescriptorSetDefaultValues m_DefaultValues;
};
}  // namespace vk
