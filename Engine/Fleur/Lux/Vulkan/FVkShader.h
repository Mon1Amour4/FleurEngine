#pragma once

#include <spirv_reflect.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include "FVkPipeline.h"

class FVkPipelineLayout;

namespace vk
{
struct FVkDescriptorBindingReflection
{
    uint32_t set = 0;
    uint32_t binding = 0;
    VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    uint32_t descriptorCount = 0;
    VkShaderStageFlags stageFlags = 0;
};

struct FVkShaderReflection
{
    std::vector<FVkDescriptorBindingReflection> descriptorBindings;
    std::vector<VkPushConstantRange> pushConstants;
};

struct ShaderCreateInfo
{
    const void* const pVertexData;
    size_t vertexSize;

    const void* const pGeometryData{nullptr};
    size_t geometrySize{0};

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

    FVkShader(const FVkShader&) = delete;
    FVkShader& operator=(const FVkShader&) = delete;
    FVkShader(FVkShader&&) = delete;
    FVkShader& operator=(FVkShader&&) = delete;

    void Init(VkDevice device, ShaderCreateInfo& info);

    inline const FVkShaderReflection& GetReflection() const
    {
        return m_Reflection;
    }

    void BuildPipeline(FVkPipeline& pipeline, const GetPipelineInfo& info,
                       const std::shared_ptr<FVkPipelineLayout>& pipelineLayout) const;
    bool isInitialized() const
    {
        return m_VertexShader.shaderModule && m_FragmentShader.shaderModule;
    }

private:
    void DestroyModules();

    VkDevice m_Device;

    ShaderData m_VertexShader;
    ShaderData m_GeometryShader;
    ShaderData m_FragmentShader;

    struct VertexInput
    {
        std::vector<VkVertexInputAttributeDescription> m_VertexInputAttributes;
        VkVertexInputBindingDescription bindingDescription;
        VkPipelineVertexInputStateCreateInfo createInfo;
    } m_VertexInput;


    std::vector<VkPushConstantRange> m_PushConstants;
    FVkShaderReflection m_Reflection;

    bool getReflection(ShaderData& shaderData, const void* const pVertexData, size_t vertexSize);
    void mergePushConstants();
    VkShaderStageFlagBits convertReflectionShaderStage(SpvReflectShaderStageFlagBits stage);
    VkDescriptorType convertReflectionDescriptorType(SpvReflectDescriptorType type);
    VkFormat convertReflectionFormat(SpvReflectFormat format);

    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;


};
}  // namespace vk
