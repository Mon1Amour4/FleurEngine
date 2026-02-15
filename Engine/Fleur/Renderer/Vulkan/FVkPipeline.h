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
    VkShaderModule vertexShader = VK_NULL_HANDLE;
    VkShaderModule fragmentShader = VK_NULL_HANDLE;

    const std::vector<VkPushConstantRange>& pushConstants;

    // Vertex input
    const SFLVertexInput* vertexInput;

    // IA
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Raster / Depth
    bool depthWriteEnable = true;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;

    // Formats (dynamic rendering)
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    // MSAA
    VkSampleCountFlagBits samplesCount = VK_SAMPLE_COUNT_1_BIT;

    // Optional
    VkExtent2D extent{};
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

        Builder& add(uint32_t idx, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t count)
        {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = idx;
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

            const VkDescriptorBindingFlagsEXT flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
                                                      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT /*|
                                                      VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT*/;

            VkDescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags{};
            binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
            binding_flags.bindingCount = m_Bindings.size();
            binding_flags.pBindingFlags = &flags;

            VkDescriptorSetLayoutCreateInfo info{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                 .pNext = &binding_flags,
                                                 .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
                                                 .bindingCount = (uint32_t)m_Bindings.size(),
                                                 .pBindings = m_Bindings.data()};

            VkDescriptorSetLayout layout;
            if (vkCreateDescriptorSetLayout(m_Device, &info, nullptr, &layout) != VK_SUCCESS)
                assert(false);

            return new FVkDescriptorSetLayout(m_Device, layout);
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

private:
    FVkDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout layout)
        : m_Device(device)
        , m_Layout(layout)
    {
    }

    VkDevice m_Device;
    VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
};

class FVkPipeline
{
public:
    FVkPipeline();
    ~FVkPipeline();

    void Init(VkDevice device, FGraphicsPipelineDesc& dedc);

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