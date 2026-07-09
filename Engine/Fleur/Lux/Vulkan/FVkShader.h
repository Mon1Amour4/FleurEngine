#pragma once

#include <spirv_reflect.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

#include <functional>
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
    VkPrimitiveTopology topology;
    VkCompareOp depthCompareOp;
    VkSampleCountFlagBits samplesCount;
    VkCullModeFlags cullMode;
    VkFrontFace frontFace;
    bool depthTestEnable;
    bool depthWriteEnable;

    VkFormat colorFormat;
    VkFormat depthFormat;

    inline bool operator==(const GetPipelineInfo& rhs) const noexcept
    {
        return topology == rhs.topology && depthCompareOp == rhs.depthCompareOp && samplesCount == rhs.samplesCount && cullMode == rhs.cullMode &&
               frontFace == rhs.frontFace && depthTestEnable == rhs.depthTestEnable && depthWriteEnable == rhs.depthWriteEnable &&
               colorFormat == rhs.colorFormat && depthFormat == rhs.depthFormat;
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
    inline const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const
    {
        return m_GlobalDescriptorSetLayouts;
    }

    inline const FVkShader::ShaderData& GetVertexShaderData() const
    {
        return m_VertexShader;
    }
    inline const FVkShader::ShaderData& GetFragmentShaderData() const
    {
        return m_FragmentShader;
    }

    FVkPipeline* GetPipeline(GetPipelineInfo& info);
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
    std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> m_GlobalDescriptorSetLayoutBindingsMap;
    std::vector<VkDescriptorSetLayout> m_GlobalDescriptorSetLayouts;

    void getReflection(ShaderData& shaderData, const void* const pVertexData, size_t vertexSize);
    void createDescriptorSetLayouts();

    VkShaderStageFlagBits convertReflectionShaderStage(SpvReflectShaderStageFlagBits stage);
    VkDescriptorType convertReflectionDescriptorType(SpvReflectDescriptorType type);
    VkFormat convertReflectionFormat(SpvReflectFormat format);

    // ---------- pipeline ----------
    std::unordered_map<GetPipelineInfo, FVkPipeline> m_PipelineCache;

    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;


    static DescriptorSetDefaultValues m_DefaultValues;
};
}  // namespace vk
