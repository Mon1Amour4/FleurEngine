#pragma once

#include <vulkan/vulkan.h>

#include <cassert>
#include <memory>

#include "VkHelper.h"
enum BindingType
{
    UNIFORM_BUFFER,
    TEXTURE2D,
    CUBEMAP,
};

struct FGraphicsPipelineDesc
{
    const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts;

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

    std::vector<VkPipelineShaderStageCreateInfo>* shaderStages = nullptr;

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

class FVkDescriptorSetLayout
{
public:
    class Builder
    {
    public:
        Builder(VkDevice device)
            : m_Device(device)
        {
        }

        Builder& add(uint32_t bindingIdx, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t count)
        {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = bindingIdx;
            binding.descriptorType = type;
            binding.descriptorCount = count;
            binding.stageFlags = stageFlags;
            binding.pImmutableSamplers = nullptr;

            m_Bindings.push_back(binding);
            return *this;
        }

        std::unique_ptr<FVkDescriptorSetLayout> build(VkDescriptorBindingFlagsEXT bindingFlags)
        {
            // One flag set per binding.
            std::vector<VkDescriptorBindingFlagsEXT> perBindingFlags(m_Bindings.size(), bindingFlags);

            VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsInfo{};
            bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
            bindingFlagsInfo.bindingCount = static_cast<uint32_t>(perBindingFlags.size());
            bindingFlagsInfo.pBindingFlags = perBindingFlags.data();

            VkDescriptorSetLayoutCreateFlags layoutFlags = 0;
            if (bindingFlags & VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT)
            {
                layoutFlags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
            }

            VkDescriptorSetLayoutCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            info.pNext = perBindingFlags.empty() ? nullptr : &bindingFlagsInfo;
            info.flags = layoutFlags;
            info.bindingCount = static_cast<uint32_t>(m_Bindings.size());
            info.pBindings = m_Bindings.data();

            VkDescriptorSetLayout layout = VK_NULL_HANDLE;
            VK_CHECK(vkCreateDescriptorSetLayout(m_Device, &info, nullptr, &layout));

            return std::unique_ptr<FVkDescriptorSetLayout>(new FVkDescriptorSetLayout(m_Device, layout, m_Bindings.size()));
        }

    private:
        VkDevice m_Device;
        std::vector<VkDescriptorSetLayoutBinding> m_Bindings;
        uint32_t m_CurrentBinding = 0;
    };

    ~FVkDescriptorSetLayout()
    {
        if (m_Layout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(m_Device, m_Layout, nullptr);
    }

    VkDescriptorSetLayout GetDescriptorSetLayout() const
    {
        return m_Layout;
    }
    inline uint32_t GetBindingCount() const
    {
        return m_BindingCount;
    }

private:
    FVkDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout layout, uint32_t bindingCount)
        : m_Device(device)
        , m_Layout(layout)
        , m_BindingCount(bindingCount)
    {
    }

    VkDevice m_Device;
    VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
    uint32_t m_BindingCount;
};

class FVkPipeline
{
public:
    FVkPipeline();
    ~FVkPipeline();

    void Init(VkDevice device, FGraphicsPipelineDesc& desc);
    void Destroy();

    VkPipeline GetPipeline() const
    {
        return m_Pipeline;
    }
    VkPipelineLayout GetPipelineLayout() const
    {
        return m_PipelineLayout;
    }
    const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const
    {
        return m_DescriptorSetLayouts;
    }

private:
    VkDevice m_Device;
    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;
    std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

    uint32_t GetBindingIdx();
};
