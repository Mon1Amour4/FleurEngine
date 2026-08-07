#include "PointLightShadowMap.h"

#include <algorithm>
#include <cassert>
#include <cstring>

#include "VkHelper.h"

namespace
{
constexpr uint32_t kCubeLayers = 6;
}

PointLightShadowMap::PointLightShadowMap(uint32_t textureCount)
    : m_TextureCount(textureCount)
    , m_PointLightShadowMaps(textureCount)
    , m_ArrayImageViews(textureCount, VK_NULL_HANDLE)
{
    assert(textureCount > 0);
}

PointLightShadowMap::~PointLightShadowMap()
{
    Destroy();
    m_PipelineLayout.reset();
}

void PointLightShadowMap::Create(VkDevice device, VkPhysicalDevice physicalDevice, FVkMemoryTracker& memoryTracker, uint32_t resolution, VkFormat depthFormat)
{
    Destroy();

    assert(device != VK_NULL_HANDLE);
    assert(physicalDevice != VK_NULL_HANDLE);
    assert(resolution > 0);
    assert(resolution <= 4096);
    assert(depthFormat != VK_FORMAT_UNDEFINED);

    m_Device = device;
    m_PhysicalDevice = physicalDevice;
    m_Extent = {resolution, resolution};
    m_DepthFormat = depthFormat;
    m_ImagesInitialized = false;

    for (uint32_t index = 0; index < m_TextureCount; ++index)
    {
        VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = m_DepthFormat;
        imageInfo.extent = {m_Extent.width, m_Extent.height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = kCubeLayers;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        m_PointLightShadowMaps[index].CreateImage(m_Device, m_PhysicalDevice, memoryTracker, FVkAllocationCategory::DepthTarget, imageInfo,
                                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
        m_PointLightShadowMaps[index].CreateImageView(VK_IMAGE_VIEW_TYPE_CUBE, kCubeLayers);

        VkImageViewCreateInfo arrayViewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        arrayViewInfo.image = m_PointLightShadowMaps[index].GetImage();
        arrayViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        arrayViewInfo.format = m_DepthFormat;
        arrayViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        arrayViewInfo.subresourceRange.levelCount = 1;
        arrayViewInfo.subresourceRange.layerCount = kCubeLayers;
        VK_CHECK(vkCreateImageView(m_Device, &arrayViewInfo, nullptr, &m_ArrayImageViews[index]));
    }

    m_MatricesBuffer.Init(m_Device, m_PhysicalDevice, memoryTracker, FVkAllocationCategory::Buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                          sizeof(ShadowMatrices) * m_TextureCount, sizeof(ShadowMatrices));

}

void PointLightShadowMap::Begin(FVkCommandBuffer& commandBuffer, uint32_t lightIndex)
{
    VkCommandBuffer rawCommandBuffer = *commandBuffer.GetCommandBuffer();
    VkCommandBuffer commandBufferHandle = rawCommandBuffer;
    assert(commandBufferHandle != VK_NULL_HANDLE);
    assert(lightIndex < m_TextureCount);
    assert(m_Pipeline != nullptr);
    assert(m_Pipeline->GetPipeline() != VK_NULL_HANDLE);
    assert(lightIndex < m_MatricesDescriptorSets.size());
    assert(m_ActiveLightIndex == UINT32_MAX);

    m_ActiveLightIndex = lightIndex;

    FVkTexture& texture = m_PointLightShadowMaps[lightIndex];
    transitionImageLayout(commandBufferHandle, texture.GetImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo depthAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depthAttachment.imageView = m_ArrayImageViews[lightIndex];
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
    renderingInfo.renderArea = {{0, 0}, m_Extent};
    renderingInfo.layerCount = kCubeLayers;
    renderingInfo.colorAttachmentCount = 0;
    renderingInfo.pColorAttachments = nullptr;
    renderingInfo.pDepthAttachment = &depthAttachment;
    vkCmdBeginRendering(commandBufferHandle, &renderingInfo);

    vkCmdBindPipeline(commandBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipeline());

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = static_cast<float>(m_Extent.height);
    viewport.width = static_cast<float>(m_Extent.width);
    viewport.height = -static_cast<float>(m_Extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBufferHandle, 0, 1, &viewport);

    VkRect2D scissor{{0, 0}, m_Extent};
    vkCmdSetScissor(commandBufferHandle, 0, 1, &scissor);

    vkCmdBindDescriptorSets(commandBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipelineLayout(), 1, 1,
                             &m_MatricesDescriptorSets[lightIndex], 0, nullptr);
}

void PointLightShadowMap::End(FVkCommandBuffer& commandBuffer)
{
    VkCommandBuffer commandBufferHandle = *commandBuffer.GetCommandBuffer();
    assert(commandBufferHandle != VK_NULL_HANDLE);

    vkCmdEndRendering(commandBufferHandle);

    assert(m_ActiveLightIndex != UINT32_MAX);
    const uint32_t lightIndex = m_ActiveLightIndex;
    transitionImageLayout(commandBufferHandle, m_PointLightShadowMaps[lightIndex].GetImage(),
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    m_ActiveLightIndex = UINT32_MAX;
}

void PointLightShadowMap::PrepareForSampling(FVkCommandBuffer& commandBuffer)
{
    VkCommandBuffer commandBufferHandle = *commandBuffer.GetCommandBuffer();
    assert(commandBufferHandle != VK_NULL_HANDLE);
    assert(m_ActiveLightIndex == UINT32_MAX);

    if (m_ImagesInitialized)
        return;

    for (uint32_t index = 0; index < m_TextureCount; ++index)
    {
        transitionImageLayout(commandBufferHandle, m_PointLightShadowMaps[index].GetImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    m_ImagesInitialized = true;
}

void PointLightShadowMap::transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout,
                                                 VkImageLayout newLayout)
{
    if (oldLayout == newLayout)
        return;

    VkAccessFlags sourceAccessMask = 0;
    VkAccessFlags destinationAccessMask = 0;
    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        destinationAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        destinationAccessMask = VK_ACCESS_SHADER_READ_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        sourceAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        destinationAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        sourceAccessMask = VK_ACCESS_SHADER_READ_BIT;
        destinationAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        assert(false && "Unsupported point-light shadow-map image layout transition");
        return;
    }

    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcAccessMask = sourceAccessMask;
    barrier.dstAccessMask = destinationAccessMask;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = kCubeLayers;

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void PointLightShadowMap::createDescriptorSet()
{
    const VkDescriptorSetLayout layout = m_PipelineLayout->GetSetLayout(1);
    assert(layout != VK_NULL_HANDLE);
    std::array<vk::abstraction::DescriptorAllocator::PoolSizeRatio, 1> poolRatios{{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
    }};
    m_DescriptorAllocator.init(m_Device, 1, poolRatios);
    m_MatricesDescriptorSets.resize(m_TextureCount);
    for (uint32_t index = 0; index < m_TextureCount; ++index)
    {
        m_MatricesDescriptorSets[index] = m_DescriptorAllocator.allocate(m_Device, layout, 1);

        const VkDeviceSize offset = sizeof(ShadowMatrices) * index;
        vk::abstraction::DescriptorWriter writer;
        writer.write_buffer(0, m_MatricesBuffer.GetBuffer(), sizeof(ShadowMatrices), offset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.update_set(m_Device, m_MatricesDescriptorSets[index]);
    }
}

void PointLightShadowMap::UpdateMatrices(uint32_t lightIndex, const std::array<Fleur::Mat4, 6>& matrices)
{
    assert(lightIndex < m_TextureCount);

    void* mappedMemory = m_MatricesBuffer.Map();
    std::memcpy(static_cast<char*>(mappedMemory) + sizeof(ShadowMatrices) * lightIndex,
                matrices.data(), sizeof(ShadowMatrices));
    m_MatricesBuffer.Unmap();
}

void PointLightShadowMap::CreatePipeline(vk::FVkShader& shader)
{
    assert(m_Device != VK_NULL_HANDLE);
    vk::GetPipelineInfo pipelineInfo{};
    pipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInfo.depthTestEnable = true;
    pipelineInfo.depthWriteEnable = true;
    pipelineInfo.cullMode = VK_CULL_MODE_NONE;
    pipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineInfo.colorAttachmentCount = 0;
    pipelineInfo.colorFormat = VK_FORMAT_UNDEFINED;
    pipelineInfo.depthFormat = m_DepthFormat;

    m_PipelineLayout = std::make_shared<FVkPipelineLayout>();
    m_PipelineLayout->Init(m_Device, shader);
    createDescriptorSet();
    m_Pipeline = &m_PipelineCache.Get(shader, pipelineInfo, m_PipelineLayout);
}

void PointLightShadowMap::Destroy()
{
    if (m_Device == VK_NULL_HANDLE)
        return;

    m_Pipeline = nullptr;

    for (VkImageView view : m_ArrayImageViews)
    {
        if (view != VK_NULL_HANDLE)
            vkDestroyImageView(m_Device, view, nullptr);
    }
    for (FVkTexture& texture : m_PointLightShadowMaps) texture.Destroy();

    m_MatricesBuffer.Destroy();
    m_DescriptorAllocator.destroy_pools(m_Device);
    std::fill(m_ArrayImageViews.begin(), m_ArrayImageViews.end(), VK_NULL_HANDLE);
    m_MatricesDescriptorSets.clear();
    m_ImagesInitialized = false;
    m_ActiveLightIndex = UINT32_MAX;
    m_Device = VK_NULL_HANDLE;
    m_PhysicalDevice = VK_NULL_HANDLE;
}
