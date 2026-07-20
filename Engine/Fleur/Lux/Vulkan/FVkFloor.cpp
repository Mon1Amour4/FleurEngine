#include "FVkFloor.h"

#include <array>

FVkFloor::~FVkFloor()
{
    if (m_Sampler)
        vkDestroySampler(m_Device, m_Sampler, nullptr);
    if (m_DescriptorPool)
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
}

void FVkFloor::Create(const FVkDevice* device, const FVkSwapchain* swapchain, vk::FVkShader* shader, VkDescriptorSetLayout cameraLayout,
                      VkImageView textureView, float height, VkSampleCountFlagBits sampleCount, VkFormat depthFormat)
{
    m_Device = device->GetLogicalDevice();
    m_Shader = shader;
    const VkExtent2D extent = swapchain->GetSwapchainExtent();
    m_Viewport = {0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};
    m_Scissor = {{0, 0}, extent};

    vk::GetPipelineInfo pipelineInfo{};
    pipelineInfo.cullMode = VK_CULL_MODE_NONE;
    pipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineInfo.depthTestEnable = false;
    pipelineInfo.depthWriteEnable = true;
    pipelineInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipelineInfo.samplesCount = sampleCount;
    pipelineInfo.colorFormat = swapchain->GetImageFormat();
    pipelineInfo.depthFormat = depthFormat;

    std::vector<VkDescriptorSetLayout> descriptorLayouts = {cameraLayout};
    m_TextureLayout = FVkDescriptorSetLayout::Builder(m_Device).add(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1).build(0);
    descriptorLayouts.push_back(m_TextureLayout->GetDescriptorSetLayout());
    m_Pipeline = &m_Shader->GetPipeline(pipelineInfo, descriptorLayouts);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    VK_CHECK(vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_Sampler));

    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};
    VkDescriptorPoolCreateInfo poolInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = 1, .poolSizeCount = 1, .pPoolSizes = &poolSize};
    VK_CHECK(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool));

    const VkDescriptorSetLayout textureLayout = m_TextureLayout->GetDescriptorSetLayout();
    VkDescriptorSetAllocateInfo allocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = m_DescriptorPool, .descriptorSetCount = 1, .pSetLayouts = &textureLayout};
    VK_CHECK(vkAllocateDescriptorSets(m_Device, &allocateInfo, &m_DescriptorSet));
    SetFloor(textureView, height);
}

void FVkFloor::SetFloor(VkImageView textureView)
{
    VkDescriptorImageInfo imageInfo{.sampler = m_Sampler, .imageView = textureView, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                               .dstSet = m_DescriptorSet,
                               .dstBinding = 0,
                               .descriptorCount = 1,
                               .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               .pImageInfo = &imageInfo};
    vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);
}

void FVkFloor::SetFloor(VkImageView textureView, float height)
{
    m_Height = height;
    VkDescriptorImageInfo imageInfo{.sampler = m_Sampler, .imageView = textureView, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                               .dstSet = m_DescriptorSet,
                               .dstBinding = 0,
                               .descriptorCount = 1,
                               .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               .pImageInfo = &imageInfo};
    vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);
}

void FVkFloor::Record(VkCommandBuffer commandBuffer, VkDescriptorSet cameraDescriptor, VkExtent2D extent, const Fleur::Graphics::SFLCameraData& cameraData)
{
    if (!m_Pipeline)
        return;

    m_Viewport.width = static_cast<float>(extent.width);
    m_Viewport.height = static_cast<float>(extent.height);
    m_Scissor.extent = extent;

    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipeline());
    vkCmdSetViewport(commandBuffer, 0, 1, &m_Viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &m_Scissor);
    const std::array<VkDescriptorSet, 2> descriptorSets{cameraDescriptor, m_DescriptorSet};
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipelineLayout(), 0, static_cast<uint32_t>(descriptorSets.size()),
                            descriptorSets.data(), 0, nullptr);

    const Fleur::Vec4 cameraPosition = Fleur::Math::inverse(cameraData.view)[3];

    PushConstants constants{};
    constants.gridParams = Fleur::Math::Vec4(m_Height, 5000.0f, cameraPosition.x, cameraPosition.z);
    vkCmdPushConstants(commandBuffer, m_Pipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &constants);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}
