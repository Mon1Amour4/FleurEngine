#pragma once

#include <vulkan/vulkan.h>

#include <cassert>

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

        FVkDescriptorSetLayout* build()
        {
            // ---------- using descriptor indexing ----------
            // ---------- https://docs.vulkan.org/samples/latest/samples/extensions/descriptor_indexing/README.html ----------

            const VkDescriptorBindingFlagsEXT flags =
                VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT /*|
                                                                VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT*/
                ;

            std::vector<VkDescriptorBindingFlagsEXT> bindingFlags(m_Bindings.size(), flags);
            VkDescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags{};
            binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
            binding_flags.bindingCount = m_Bindings.size();
            binding_flags.pBindingFlags = bindingFlags.data();

            VkDescriptorSetLayoutCreateInfo info{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                 .pNext = &binding_flags,
                                                 .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
                                                 .bindingCount = (uint32_t)m_Bindings.size(),
                                                 .pBindings = m_Bindings.data()};

            VkDescriptorSetLayout layout;
            if (vkCreateDescriptorSetLayout(m_Device, &info, nullptr, &layout) != VK_SUCCESS)
                assert(false);

            return new FVkDescriptorSetLayout(m_Device, layout, m_Bindings.size());
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
    std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

    uint32_t GetBindingIdx();
};