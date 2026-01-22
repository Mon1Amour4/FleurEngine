#pragma once

#include <vulkan/vulkan.h>

#include <vector>

class FVkCommandPool
{
public:
    FVkCommandPool();
    ~FVkCommandPool();
    void Init(VkDevice device, VkCommandPoolCreateFlagBits usage, uint32_t queueFamilyIndex);

    inline VkCommandPool Pool() const
    {
        return m_CommandPool;
    }

    int AddCommandBuffer(VkCommandBufferLevel level, uint32_t count);
    VkCommandBuffer GetCommandBuffer(VkCommandBufferLevel level, uint32_t idx);

private:
    VkCommandPool m_CommandPool;
    VkDevice m_Device;
    VkCommandPoolCreateFlagBits m_Usage;
    int m_QueueFamilyIndex;

    std::vector<VkCommandBuffer> m_PrimaryCommandBuffers;
    std::vector<VkCommandBuffer> m_SecondaryCommandBuffers;
};