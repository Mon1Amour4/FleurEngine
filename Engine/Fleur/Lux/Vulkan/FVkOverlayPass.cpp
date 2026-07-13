#include "FVkOverlayPass.h"

FVkOverlayPass::~FVkOverlayPass()
{
    delete m_Pipeline;
}

void FVkOverlayPass::Create(const FVkDevice* device, const FVkSwapchain* swapchain, VkDescriptorSetLayout texturesLayout, VkDescriptorSet texturesDescriptorSet,
                            uint32_t shadowMapTextureSlot, VkSampleCountFlagBits sampleCount, VkFormat depthFormat, uint32_t /*framesInFlight*/)
{
    m_Device = device->GetLogicalDevice();
    m_PhysicalDevice = device->GetPhysicalDevice();
    m_TexturesLayout = texturesLayout;
    m_TexturesDescriptorSet = texturesDescriptorSet;
    m_ShadowMapTextureSlot = shadowMapTextureSlot;
    m_ColorFormat = swapchain->GetImageFormat();
    m_DepthFormat = depthFormat;
    m_SampleCount = sampleCount;

    uint32_t frameCount = swapchain->GetSwapchainImageCount();
    m_Buffers.reserve(frameCount);
    for (size_t i = 0; i < frameCount; i++)
    {
        FVkBuffer& buffer = m_Buffers.emplace_back();
        buffer.Init(m_Device, m_PhysicalDevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    sizeof(OverlayVertex) * kMaxVertsPerFrame, sizeof(OverlayVertex));
    }

    m_Initialized = true;
}

void FVkOverlayPass::SetShader(vk::FVkShader* overlayShader)
{
    m_Shader = overlayShader;

    delete m_Pipeline;
    m_Pipeline = nullptr;

    if (!m_Shader)
        return;

    vk::GetPipelineInfo pipelineInfo{};
    pipelineInfo.blendEnable = true;
    pipelineInfo.cullMode = VK_CULL_MODE_NONE;
    pipelineInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    pipelineInfo.depthTestEnable = false;
    pipelineInfo.depthWriteEnable = false;
    pipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineInfo.samplesCount = m_SampleCount;
    pipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInfo.colorFormat = m_ColorFormat;
    pipelineInfo.depthFormat = m_DepthFormat;

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{m_TexturesLayout};
    m_Pipeline = m_Shader->GetPipeline(pipelineInfo, descriptorSetLayouts);
    assert(m_Pipeline);
}

void FVkOverlayPass::AddQuad(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d, glm::vec4 color)
{
    m_Geometry.push_back({glm::vec3(a, 0.0f), glm::vec2(0, 0)});
    m_Geometry.push_back({glm::vec3(b, 0.0f), glm::vec2(1, 0)});
    m_Geometry.push_back({glm::vec3(c, 0.0f), glm::vec2(1, 1)});
    m_Geometry.push_back({glm::vec3(c, 0.0f), glm::vec2(1, 1)});
    m_Geometry.push_back({glm::vec3(d, 0.0f), glm::vec2(0, 1)});
    m_Geometry.push_back({glm::vec3(a, 0.0f), glm::vec2(0, 0)});
    m_Materials.push_back({-1, 0, color});
    m_DrawInfos.push_back({6, static_cast<uint32_t>(m_Geometry.size() - 6), static_cast<uint32_t>(m_Materials.size() - 1)});
}

void FVkOverlayPass::AddQuad(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d, uint32_t textureIdx)
{
    m_Geometry.push_back({glm::vec3(a, 0.0f), glm::vec2(0, 0)});
    m_Geometry.push_back({glm::vec3(b, 0.0f), glm::vec2(1, 0)});
    m_Geometry.push_back({glm::vec3(c, 0.0f), glm::vec2(1, 1)});
    m_Geometry.push_back({glm::vec3(c, 0.0f), glm::vec2(1, 1)});
    m_Geometry.push_back({glm::vec3(d, 0.0f), glm::vec2(0, 1)});
    m_Geometry.push_back({glm::vec3(a, 0.0f), glm::vec2(0, 0)});
    m_Materials.push_back({static_cast<int32_t>(textureIdx), 1});
    m_DrawInfos.push_back({6, static_cast<uint32_t>(m_Geometry.size() - 6), static_cast<uint32_t>(m_Materials.size() - 1)});
}

void FVkOverlayPass::AddTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec4 color)
{
    m_Geometry.push_back({glm::vec3(a, 0.0f), glm::vec2(0, 0)});
    m_Geometry.push_back({glm::vec3(b, 0.0f), glm::vec2(1, 0)});
    m_Geometry.push_back({glm::vec3(c, 0.0f), glm::vec2(0.5f, 1)});
    m_Materials.push_back({-1, 0, color});
    m_DrawInfos.push_back({3, static_cast<uint32_t>(m_Geometry.size() - 3), static_cast<uint32_t>(m_Materials.size() - 1)});
}

void FVkOverlayPass::AddTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, uint32_t textureIdx)
{
    m_Geometry.push_back({glm::vec3(a, 0.0f), glm::vec2(0, 0)});
    m_Geometry.push_back({glm::vec3(b, 0.0f), glm::vec2(1, 0)});
    m_Geometry.push_back({glm::vec3(c, 0.0f), glm::vec2(0.5f, 1)});
    m_Materials.push_back({static_cast<int32_t>(textureIdx), 1});
    m_DrawInfos.push_back({3, static_cast<uint32_t>(m_Geometry.size() - 3), static_cast<uint32_t>(m_Materials.size() - 1)});
}

void FVkOverlayPass::AddShadowMapQuad(glm::vec2 min, glm::vec2 max)
{
    AddQuad(glm::vec2(min.x, min.y), glm::vec2(max.x, min.y), glm::vec2(max.x, max.y), glm::vec2(min.x, max.y), m_ShadowMapTextureSlot);
    m_Materials.back().textureSource = 2;
}

void FVkOverlayPass::Record(FVkCommandBuffer& cmd, uint32_t frameIndex)
{
    if (m_Geometry.empty() || !m_Pipeline)
        return;

    m_Buffers[frameIndex].MemCopy(m_Geometry.data(), m_Geometry.size() * sizeof(OverlayVertex));
    cmd.BindPipeline(m_Pipeline->GetPipeline());
    cmd.BindVertexBuffer(&m_Buffers[frameIndex].GetBuffer());
    cmd.BindDescriptorSets(m_Pipeline->GetPipelineLayout(), &m_TexturesDescriptorSet, 1);
    for (const auto& drawInfo : m_DrawInfos)
    {
        const auto& material = m_Materials[drawInfo.materialIdx];
        OverlayPushConstant pushConstant{};
        pushConstant.params.x = material.textureIdx;
        pushConstant.params.y = material.textureSource;
        pushConstant.color = material.color;
        cmd.PushConstant(m_Pipeline->GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, pushConstant);
        cmd.Draw(drawInfo.vertexCount, drawInfo.vertexOffset);
    }
}

void FVkOverlayPass::Clear()
{
    m_Geometry.clear();
    m_Materials.clear();
    m_DrawInfos.clear();
}
