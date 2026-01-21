#pragma once

#include <vulkan/vulkan.h>

class FVkCmdBuffer
{
public:


private:

};

class FVkCommandPool
{
public:
    FVkCommandPool();
    ~FVkCommandPool();
    void Init(VkDevice device, VkCommandPoolCreateFlagBits usage, uint32_t queueFamilyIndex);

    inline VkCommandPool Pool() const
    {
        return m_CmdPool;
    }

private:
    VkCommandPool m_CmdPool;
    VkDevice m_Device;
    VkCommandPoolCreateFlagBits m_Usage;
    int m_QueueFamilyIndex;
};