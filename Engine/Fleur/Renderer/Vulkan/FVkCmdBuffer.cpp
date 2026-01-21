#include <cassert>

#include "FVkCmdBuffer.h"

FVkCommandPool::FVkCommandPool()
    : m_Device(nullptr)
    , m_Usage(VK_COMMAND_POOL_CREATE_FLAG_BITS_MAX_ENUM)
    , m_QueueFamilyIndex(-1)
{
}

FVkCommandPool::~FVkCommandPool()
{
    if (m_CmdPool)
        vkDestroyCommandPool(m_Device, m_CmdPool, nullptr);
}

void FVkCommandPool::Init(VkDevice device, VkCommandPoolCreateFlagBits usage, uint32_t queueFamilyIndex)
{
    m_Device = device;
    m_Usage = usage;
    m_QueueFamilyIndex = queueFamilyIndex;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = m_Usage;
    poolInfo.queueFamilyIndex = m_QueueFamilyIndex;

    if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CmdPool) != VK_SUCCESS)
    {
        assert(false);
    }
}
