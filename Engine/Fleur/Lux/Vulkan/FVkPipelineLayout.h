#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "FVkShader.h"

class FVkPipelineLayout
{
public:
    FVkPipelineLayout() = default;
    ~FVkPipelineLayout();

    FVkPipelineLayout(const FVkPipelineLayout&) = delete;
    FVkPipelineLayout& operator=(const FVkPipelineLayout&) = delete;

    void Init(VkDevice device, const vk::FVkShader& shader);
    void Destroy();

    VkPipelineLayout Get() const
    {
        return m_PipelineLayout;
    }

    VkDescriptorSetLayout GetSetLayout(uint32_t set) const
    {
        return set < m_SetLayouts.size() ? m_SetLayouts[set] : VK_NULL_HANDLE;
    }

    const std::vector<VkDescriptorSetLayout>& GetSetLayouts() const
    {
        return m_SetLayouts;
    }

private:
    static std::vector<VkPushConstantRange> NormalizePushConstants(const std::vector<VkPushConstantRange>& ranges);
    VkDescriptorSetLayout CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings, bool updateAfterBind) const;

    VkDevice m_Device{VK_NULL_HANDLE};
    VkPipelineLayout m_PipelineLayout{VK_NULL_HANDLE};
    std::vector<VkDescriptorSetLayout> m_SetLayouts;
};
