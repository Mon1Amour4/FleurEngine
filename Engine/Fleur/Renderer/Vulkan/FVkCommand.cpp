#include "FVkCommand.h"

#include <cassert>

#include "VkHelper.h"
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
    vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &m_CommandBuffer);
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

void FVkCommandBuffer::PushConstant(VkPipelineLayout pipelineLayout, VkImageAspectFlags shaderStage, SFLPushConstant constant)
{
    vkCmdPushConstants(m_CommandBuffer, pipelineLayout, shaderStage, 0, sizeof(SFLPushConstant), &constant);
}

void FVkCommandBuffer::DrawIndexed(uint32_t indexCount, size_t indexOffset, size_t vertexOffset)
{
    vkCmdDrawIndexed(m_CommandBuffer, indexCount, 1, indexOffset, vertexOffset, 0);
}


//======================================================================
// FVkSingleTimeCommandBuffer
FVkSingleTimeCommandBuffer::FVkSingleTimeCommandBuffer(VkDevice device, VkCommandPool pool)
    : m_Device(device)
    , m_CommandPool(pool)
{
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(m_Device, &fenceInfo, nullptr, &m_Fence) != VK_SUCCESS)
        assert(false);
    vkResetFences(m_Device, 1, &m_Fence);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(m_Device, &allocInfo, &m_CommandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
}

FVkSingleTimeCommandBuffer::~FVkSingleTimeCommandBuffer()
{
    vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX);
    vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &m_CommandBuffer);
    vkDestroyFence(m_Device, m_Fence, nullptr);
}

void FVkSingleTimeCommandBuffer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                                                       VkImageAspectFlags aspectMask, uint32_t mimMapLevel)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mimMapLevel;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    FindBarrierAccessMask(oldLayout, newLayout, barrier.srcAccessMask, barrier.dstAccessMask, sourceStage, destinationStage);

    vkCmdPipelineBarrier(m_CommandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void FVkSingleTimeCommandBuffer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(m_CommandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void FVkSingleTimeCommandBuffer::GenerateMipMaps(VkPhysicalDevice physicalDevice, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight,
                                                 uint32_t mipLevels)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
    assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;
    for (uint32_t i = 1; i < mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(m_CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(m_CommandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(m_CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(m_CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}
void FVkSingleTimeCommandBuffer::Submit(VkQueue queue)
{
    vkEndCommandBuffer(m_CommandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_CommandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, m_Fence);
}
