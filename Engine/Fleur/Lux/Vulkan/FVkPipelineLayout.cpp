#include "FVkPipelineLayout.h"

#include <algorithm>
#include <cassert>

FVkPipelineLayout::~FVkPipelineLayout()
{
    Destroy();
}

void FVkPipelineLayout::Destroy()
{
    if (m_Device != VK_NULL_HANDLE)
    {
        if (m_PipelineLayout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);

        for (VkDescriptorSetLayout layout : m_SetLayouts)
        {
            if (layout != VK_NULL_HANDLE)
                vkDestroyDescriptorSetLayout(m_Device, layout, nullptr);
        }
    }

    m_SetLayouts.clear();
    m_PipelineLayout = VK_NULL_HANDLE;
    m_Device = VK_NULL_HANDLE;
}

VkDescriptorSetLayout FVkPipelineLayout::CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings, bool updateAfterBind) const
{
    std::vector<VkDescriptorBindingFlags> bindingFlags;
    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    if (updateAfterBind && !bindings.empty())
    {
        bindingFlags.assign(bindings.size(), VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT);
        bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
        bindingFlagsInfo.pBindingFlags = bindingFlags.data();
    }

    VkDescriptorSetLayoutCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = (updateAfterBind && !bindings.empty()) ? &bindingFlagsInfo : nullptr,
        .flags = updateAfterBind ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT : VkDescriptorSetLayoutCreateFlags{0},
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.empty() ? nullptr : bindings.data(),
    };

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorSetLayout(m_Device, &createInfo, nullptr, &layout));
    return layout;
}

std::vector<VkPushConstantRange> FVkPipelineLayout::NormalizePushConstants(const std::vector<VkPushConstantRange>& ranges)
{
    if (ranges.empty())
        return {};

    std::vector<uint32_t> boundaries;
    boundaries.reserve(ranges.size() * 2);
    for (const VkPushConstantRange& range : ranges)
    {
        if (range.size == 0 || range.offset % 4 != 0 || range.size % 4 != 0)
        {
            assert(false && "FVkPipelineLayout: invalid push constant range alignment");
            return {};
        }

        boundaries.push_back(range.offset);
        boundaries.push_back(range.offset + range.size);
    }

    std::sort(boundaries.begin(), boundaries.end());
    boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());

    std::vector<VkPushConstantRange> normalized;
    for (size_t i = 0; i + 1 < boundaries.size(); ++i)
    {
        const uint32_t begin = boundaries[i];
        const uint32_t end = boundaries[i + 1];
        VkShaderStageFlags stages = 0;

        for (const VkPushConstantRange& range : ranges)
        {
            const uint32_t rangeEnd = range.offset + range.size;
            if (range.offset <= begin && end <= rangeEnd)
                stages |= range.stageFlags;
        }

        if (stages == 0)
            continue;

        if (!normalized.empty() && normalized.back().offset + normalized.back().size == begin && normalized.back().stageFlags == stages)
        {
            normalized.back().size += end - begin;
        }
        else
        {
            normalized.push_back(VkPushConstantRange{.stageFlags = stages, .offset = begin, .size = end - begin});
        }
    }

    return normalized;
}

void FVkPipelineLayout::Init(VkDevice device, const vk::FVkShader& shader)
{
    Destroy();
    if (device == VK_NULL_HANDLE)
        throw std::invalid_argument("FVkPipelineLayout: device is null");

    m_Device = device;
    const auto& reflection = shader.GetReflection();

    uint32_t maxSet = 0;
    bool hasBindings = false;
    for (const auto& binding : reflection.descriptorBindings)
    {
        maxSet = std::max(maxSet, binding.set);
        hasBindings = true;
    }

    m_SetLayouts.resize(hasBindings ? maxSet + 1 : 0, VK_NULL_HANDLE);
    for (uint32_t set = 0; set < m_SetLayouts.size(); ++set)
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        for (const auto& reflected : reflection.descriptorBindings)
        {
            if (reflected.set != set)
                continue;

            bindings.push_back(VkDescriptorSetLayoutBinding{
                .binding = reflected.binding,
                .descriptorType = reflected.descriptorType,
                .descriptorCount = reflected.descriptorCount,
                .stageFlags = reflected.stageFlags,
                .pImmutableSamplers = nullptr,
            });
        }

        std::sort(bindings.begin(), bindings.end(), [](const auto& lhs, const auto& rhs) { return lhs.binding < rhs.binding; });
        const bool updateAfterBind = std::any_of(bindings.begin(), bindings.end(), [](const auto& binding) { return binding.descriptorCount > 1; });
        m_SetLayouts[set] = CreateDescriptorSetLayout(bindings, updateAfterBind);
    }

    const std::vector<VkPushConstantRange> pushConstants = NormalizePushConstants(reflection.pushConstants);
    VkPipelineLayoutCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(m_SetLayouts.size()),
        .pSetLayouts = m_SetLayouts.empty() ? nullptr : m_SetLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size()),
        .pPushConstantRanges = pushConstants.empty() ? nullptr : pushConstants.data(),
    };

    VK_CHECK(vkCreatePipelineLayout(m_Device, &createInfo, nullptr, &m_PipelineLayout));
}
