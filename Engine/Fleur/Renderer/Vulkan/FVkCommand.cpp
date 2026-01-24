#include "FVkCommand.h"

#include <cassert>

#include "vk_mem_alloc.h"
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


//======================================================================
// CommandBuffer
FVkCommandBuffer::FVkCommandBuffer()
    : m_Device(nullptr)
    , m_Level(VK_COMMAND_BUFFER_LEVEL_MAX_ENUM)
    , m_CommandPool(nullptr)
    , m_CommandBuffer(nullptr)
    , m_Valid(false)
{
}
FVkCommandBuffer::~FVkCommandBuffer()
{
    vkResetCommandPool(m_Device, m_CommandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

void FVkCommandBuffer::Init(VkDevice device, VkCommandPool pool, VkCommandBufferLevel level)
{
    m_Device = device;
    m_CommandPool = pool;
    m_Level = level;

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(m_Device, &allocInfo, &m_CommandBuffer) != VK_SUCCESS)
    {
        assert(false);
    }
}

void FVkCommandBuffer::Reset()
{
    vkResetCommandBuffer(m_CommandBuffer, 0);
    m_Valid = false;
}

void FVkCommandBuffer::Begin(VkRenderPass renderPass)
{
    VkCommandBufferInheritanceInfo inheritanceInfo{};
    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.pNext = NULL;
    inheritanceInfo.renderPass = renderPass;
    inheritanceInfo.subpass = 0;
    inheritanceInfo.framebuffer = VK_NULL_HANDLE;


    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    beginInfo.pInheritanceInfo = &inheritanceInfo;

    if (vkBeginCommandBuffer(m_CommandBuffer, &beginInfo) != VK_SUCCESS)
    {
        assert(false);
    }
}
void FVkCommandBuffer::End()
{
    if (vkEndCommandBuffer(m_CommandBuffer) != VK_SUCCESS)
    {
        assert(false);
    }
    m_Valid = true;
}

void FVkCommandBuffer::BindPipeline(VkPipeline pipeline)
{
    vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void FVkCommandBuffer::BeginRenderPass(VkRenderPassBeginInfo info, VkSubpassContents content)
{
    vkCmdBeginRenderPass(m_CommandBuffer, &info, content);
}

void FVkCommandBuffer::EndRenderPass()
{
    vkCmdEndRenderPass(m_CommandBuffer);
}

void FVkCommandBuffer::ExecuteSecondaryCommandBuffer(VkCommandBuffer* secondary)
{
    vkCmdExecuteCommands(m_CommandBuffer, 1, secondary);
}

void FVkCommandBuffer::SetViewport(VkViewport viewport)
{
    vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
}

void FVkCommandBuffer::SetScissors(VkRect2D scissors)
{
    vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissors);
}

void FVkCommandBuffer::BindVertexBuffer(VkBuffer* buffer)
{
    VkDeviceSize offsets{0};
    vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, buffer, &offsets);
}

void FVkCommandBuffer::BindIndexBuffer(VkBuffer* buffer, VkIndexType indextype)
{
    vkCmdBindIndexBuffer(m_CommandBuffer, *buffer, 0, indextype);
}

void FVkCommandBuffer::BindDescriptorSet(VkPipelineLayout pipelineLayout, VkDescriptorSet* descriptorSet)
{
    vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, descriptorSet, 0, nullptr);
}

void FVkCommandBuffer::PushConstant(VkPipelineLayout pipelineLayout, VkShaderStageFlagBits shaderStage, SFLPushConstant constant)
{
    vkCmdPushConstants(m_CommandBuffer, pipelineLayout, shaderStage, 0, sizeof(SFLPushConstant), &constant);
}

void FVkCommandBuffer::DrawIndexed(uint32_t indexCount, size_t indexOffset, size_t vertexOffset)
{
    vkCmdDrawIndexed(m_CommandBuffer, indexCount, 1, indexOffset, vertexOffset, 0);
}
