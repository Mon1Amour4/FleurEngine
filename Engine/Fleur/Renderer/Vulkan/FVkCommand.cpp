#include "FVkCommand.h"

#include <cassert>

//======================================================================
// CommandPool
FVkCommandPool::FVkCommandPool()
    : m_Device(nullptr)
    , m_Usage(VK_COMMAND_POOL_CREATE_FLAG_BITS_MAX_ENUM)
    , m_QueueFamilyIndex(-1)
    , m_CommandPool(nullptr)
{
}
FVkCommandPool::~FVkCommandPool()
{
    if (m_CommandPool)
        vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
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

    if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
    {
        assert(false);
    }
}

int FVkCommandPool::AddCommandBuffer(VkCommandBufferLevel level, uint32_t count)
{
    if (count == 0 || (level & VK_COMMAND_BUFFER_LEVEL_MAX_ENUM))
        return -1;

    std::vector<VkCommandBuffer>* vec = nullptr;
    int idx = -1;
    if (level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
    {
        idx = m_PrimaryCommandBuffers.size();
        vec = &m_PrimaryCommandBuffers;
    }
    else if (level & VK_COMMAND_BUFFER_LEVEL_SECONDARY)
    {
        idx = m_SecondaryCommandBuffers.size();
        vec = &m_SecondaryCommandBuffers;
    }

    for (size_t i = 0; i < count; i++)
    {
        auto& buffer = vec->emplace_back();

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.level = level;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(m_Device, &allocInfo, &buffer) != VK_SUCCESS)
        {
            assert(false);
        }
    }

    // return idx for first added buffer
    return idx + 1;
}

VkCommandBuffer FVkCommandPool::GetCommandBuffer(VkCommandBufferLevel level, uint32_t idx)
{
    assert(level & VK_COMMAND_BUFFER_LEVEL_MAX_ENUM);

    if (level & VK_COMMAND_BUFFER_LEVEL_PRIMARY)
    {
        assert(idx <= m_PrimaryCommandBuffers.size() - 1);

        return m_PrimaryCommandBuffers[idx];
    }
    else if (level & VK_COMMAND_BUFFER_LEVEL_SECONDARY)
    {
        assert(idx <= m_SecondaryCommandBuffers.size() - 1);

        return m_SecondaryCommandBuffers[idx];
    }
}
