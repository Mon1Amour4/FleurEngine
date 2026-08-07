#include "FVkFloor.h"

FVkFloor::~FVkFloor()
{
}

void FVkFloor::Create(const FVkDevice* device, const FVkSwapchain* swapchain, vk::FVkShader* shader, VkImageView textureView, float height,
                      VkSampleCountFlagBits sampleCount, VkFormat depthFormat)
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

    m_PipelineLayout = std::make_shared<FVkPipelineLayout>();
    m_PipelineLayout->Init(m_Device, *m_Shader);
    m_Pipeline = &m_PipelineCache.Get(*m_Shader, pipelineInfo, m_PipelineLayout);

    SetFloor(textureView, height);
}

void FVkFloor::SetFloor(VkImageView textureView)
{
    (void)textureView;
}

void FVkFloor::SetFloor(VkImageView textureView, float height)
{
    m_Height = height;
    (void)textureView;
}

void FVkFloor::Record(VkCommandBuffer commandBuffer, VkDescriptorSet cameraDescriptor, VkExtent2D extent, const Fleur::Graphics::SFLCameraData& cameraData)
{
    if (!m_Pipeline)
        return;

    m_Viewport.width = static_cast<float>(extent.width);
    m_Viewport.height = static_cast<float>(extent.height);
    m_Scissor.extent = extent;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipeline());
    vkCmdSetViewport(commandBuffer, 0, 1, &m_Viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &m_Scissor);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipelineLayout(), 0, 1, &cameraDescriptor, 0, nullptr);

    const Fleur::Vec4 cameraPosition = Fleur::Math::inverse(cameraData.view)[3];

    PushConstants constants{};
    constants.gridParams = Fleur::Math::Vec4(m_Height, 5000.0f, cameraPosition.x, cameraPosition.z);
    vkCmdPushConstants(commandBuffer, m_Pipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &constants);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}
