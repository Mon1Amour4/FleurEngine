#include "FVkDebugDraw.h"

FVkDebugDraw::~FVkDebugDraw()
{
    delete m_LinePipeline;
    delete m_PointPipeline;
    delete m_QuadPipeline;

    m_LineBuffers.clear();
    m_LineBuffers.shrink_to_fit();

    m_PointBuffers.clear();
    m_PointBuffers.shrink_to_fit();
}

void FVkDebugDraw::Create(const FVkDevice* device, const FVkSwapchain* swapchain, vk::FVkShader* primitivesShader, vk::FVkShader* geometryShader,
                          VkDescriptorSetLayout geometryTexturesLayout, VkDescriptorSet geometryTexturesDescriptorSet, VkSampleCountFlagBits sampleCount,
                          VkFormat depthFormat, uint32_t framesInFlight)
{
    m_Device = device->GetLogicalDevice();
    m_PhysicalDevice = device->GetPhysicalDevice();
    m_PrimitivesShader = primitivesShader;
    m_GeometryShader = geometryShader;
    m_GeometryTexturesLayout = geometryTexturesLayout;
    m_GeometryTexturesDescriptorSet = geometryTexturesDescriptorSet;
    m_ColorFormat = swapchain->GetImageFormat();
    m_Extent = swapchain->GetSwapchainExtent();
    m_SampleCount = sampleCount;
    m_DepthFormat = depthFormat;
    m_FramesInFlight = framesInFlight;

    // TODO: allocate per-frame vertex buffers (line + point), capacity kVertexBufferSize.
    //   m_LineBuffers.resize(framesInFlight);
    //   for (auto& b : m_LineBuffers) { b = new FVkBuffer(); b->Init(m_Device, m_PhysicalDevice,
    //       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, kVertexBufferSize, sizeof(SDebugVertex)); }
    //   m_PointBuffers likewise.

    uint32_t frameCount = swapchain->GetSwapchainImageCount();
    m_LineBuffers.reserve(frameCount);
    m_PointBuffers.reserve(frameCount);
    m_GeometryBuffers.reserve(frameCount);
    for (size_t i = 0; i < frameCount; i++)
    {
        FVkBuffer& lineBuffer = m_LineBuffers.emplace_back();
        lineBuffer.Init(m_Device, m_PhysicalDevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, kVertexBufferSize,
                        kVertexBufferStride);

        FVkBuffer& pointBuffer = m_PointBuffers.emplace_back();
        pointBuffer.Init(m_Device, m_PhysicalDevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, kVertexBufferSize,
                         kVertexBufferStride);

        FVkBuffer& geometryBuffer = m_GeometryBuffers.emplace_back();
        geometryBuffer.Init(m_Device, m_PhysicalDevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                            sizeof(SDebugVertex) * kMaxVertsPerFrame, sizeof(SDebugVertex));
    }

    createPipelines();

    m_Initialized = true;
}

void FVkDebugDraw::createPipelines()
{
    if (m_PrimitivesShader)
    {
        if (!m_PrimitivesShader->isInitialized())
        {
            assert(false);
        }

        vk::GetPipelineInfo linePipelineInfo{};
        linePipelineInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        linePipelineInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        linePipelineInfo.depthTestEnable = true;
        linePipelineInfo.depthWriteEnable = true;
        linePipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        linePipelineInfo.samplesCount = m_SampleCount;
        linePipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        linePipelineInfo.colorFormat = m_ColorFormat;
        linePipelineInfo.depthFormat = m_DepthFormat;

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        m_LinePipeline = m_PrimitivesShader->GetPipeline(linePipelineInfo, descriptorSetLayouts);
        assert(m_LinePipeline);

        vk::GetPipelineInfo pointPipelineInfo{};
        pointPipelineInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        pointPipelineInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        pointPipelineInfo.depthTestEnable = true;
        pointPipelineInfo.depthWriteEnable = true;
        pointPipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        pointPipelineInfo.samplesCount = m_SampleCount;
        pointPipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        pointPipelineInfo.colorFormat = m_ColorFormat;
        pointPipelineInfo.depthFormat = m_DepthFormat;

        m_PointPipeline = m_PrimitivesShader->GetPipeline(pointPipelineInfo, descriptorSetLayouts);
        assert(m_PointPipeline);
    }

    if (m_GeometryShader)
    {
        if (!m_GeometryShader->isInitialized())
        {
            assert(false);
        }

        vk::GetPipelineInfo quadPipelineInfo{};
        quadPipelineInfo.blendEnable = true;
        quadPipelineInfo.cullMode = VK_CULL_MODE_NONE;
        quadPipelineInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        quadPipelineInfo.depthTestEnable = true;
        quadPipelineInfo.depthWriteEnable = true;
        quadPipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        quadPipelineInfo.samplesCount = m_SampleCount;
        quadPipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        quadPipelineInfo.colorFormat = m_ColorFormat;
        quadPipelineInfo.depthFormat = m_DepthFormat;

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{m_GeometryTexturesLayout};
        m_QuadPipeline = m_GeometryShader->GetPipeline(quadPipelineInfo, descriptorSetLayouts);
        assert(m_QuadPipeline);
    }
}

static uint32_t to8(float v)
{
    v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    return static_cast<uint32_t>(v * 255.0f);
}
static uint32_t packColor(glm::vec3 c)
{
    return to8(c.r) | (to8(c.g) << 8) | (to8(c.b) << 16) | 0xFF000000u;
}

void FVkDebugDraw::AddLine(glm::vec3 a, glm::vec3 b, glm::vec3 color)
{
    m_Lines.push_back({a, glm::vec4(color, 1.f)});
    m_Lines.push_back({b, glm::vec4(color, 1.f)});
}

void FVkDebugDraw::AddPoint(glm::vec3 p, glm::vec3 color, float size)
{
    // TODO: point size — needs a push-constant or per-vertex size field (SDebugVertex has none yet).
    m_Points.push_back({p, glm::vec4(color, 1.f)});
}

void FVkDebugDraw::AddQuad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec4 color)
{
    m_Quads.push_back({a, glm::vec2(0, 0)});
    m_Quads.push_back({b, glm::vec2(1, 0)});
    m_Quads.push_back({c, glm::vec2(1, 1)});
    m_Quads.push_back({c, glm::vec2(1, 1)});
    m_Quads.push_back({d, glm::vec2(0, 1)});
    m_Quads.push_back({a, glm::vec2(0, 0)});
    m_GeometryMaterials.push_back({-1, color});
    m_GeometryDrawInfos.push_back({6, static_cast<uint32_t>(m_Quads.size() - 6), static_cast<uint32_t>(m_GeometryMaterials.size() - 1)});
}

void FVkDebugDraw::AddQuad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, uint32_t textureIdx)
{
    m_Quads.push_back({a, glm::vec2(0, 0)});
    m_Quads.push_back({b, glm::vec2(1, 0)});
    m_Quads.push_back({c, glm::vec2(1, 1)});
    m_Quads.push_back({c, glm::vec2(1, 1)});
    m_Quads.push_back({d, glm::vec2(0, 1)});
    m_Quads.push_back({a, glm::vec2(0, 0)});
    m_GeometryMaterials.push_back({static_cast<int32_t>(textureIdx)});
    m_GeometryDrawInfos.push_back({6, static_cast<uint32_t>(m_Quads.size() - 6), static_cast<uint32_t>(m_GeometryMaterials.size() - 1)});
}

void FVkDebugDraw::AddBillboard(glm::vec3 center, glm::vec2 size, uint32_t textureIdx)
{
    m_Billboards.push_back({center, size, textureIdx});
}

void FVkDebugDraw::Record(FVkCommandBuffer& cmd, const Fleur::Graphics::SFLCameraData& cameraData, uint32_t frameIndex)
{
    glm::mat4 viewProj = cameraData.proj * cameraData.view;

    if (!m_Lines.empty())
    {
        m_LineBuffers[frameIndex].MemCopy(m_Lines.data(), m_Lines.size() * sizeof(SDebugVertex));
        cmd.BindPipeline(m_LinePipeline->GetPipeline());
        cmd.BindVertexBuffer(&m_LineBuffers[frameIndex].GetBuffer());
        cmd.PushConstant(m_LinePipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, viewProj);
        cmd.Draw(static_cast<uint32_t>(m_Lines.size()), 0);
    }

    if (!m_Points.empty())
    {
        m_PointBuffers[frameIndex].MemCopy(m_Points.data(), m_Points.size() * sizeof(SDebugVertex));
        cmd.BindPipeline(m_PointPipeline->GetPipeline());
        cmd.BindVertexBuffer(&m_PointBuffers[frameIndex].GetBuffer());
        cmd.PushConstant(m_PointPipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, viewProj);
        cmd.Draw(static_cast<uint32_t>(m_Points.size()), 0);
    }
    if (!m_Quads.empty() || !m_Billboards.empty())
    {
        std::vector<GeometryVertex> geometry = m_Quads;
        std::vector<PrimitiveGeometryDrawInfo> drawInfos = m_GeometryDrawInfos;

        if (!m_Billboards.empty())
        {
            glm::mat4 invView = glm::inverse(cameraData.view);
            glm::vec3 right = glm::normalize(glm::vec3(invView[0]));
            glm::vec3 up = glm::normalize(glm::vec3(invView[1]));

            for (const auto& billboard : m_Billboards)
            {
                glm::vec3 halfRight = right * (billboard.size.x * 0.5f);
                glm::vec3 halfUp = up * (billboard.size.y * 0.5f);

                glm::vec3 a = billboard.center - halfRight + halfUp;
                glm::vec3 b = billboard.center + halfRight + halfUp;
                glm::vec3 c = billboard.center + halfRight - halfUp;
                glm::vec3 d = billboard.center - halfRight - halfUp;

                uint32_t vertexOffset = static_cast<uint32_t>(geometry.size());
                geometry.push_back({a, glm::vec2(0, 0)});
                geometry.push_back({b, glm::vec2(1, 0)});
                geometry.push_back({c, glm::vec2(1, 1)});
                geometry.push_back({c, glm::vec2(1, 1)});
                geometry.push_back({d, glm::vec2(0, 1)});
                geometry.push_back({a, glm::vec2(0, 0)});

                m_GeometryMaterials.push_back({static_cast<int32_t>(billboard.textureIdx)});
                drawInfos.push_back({6, vertexOffset, static_cast<uint32_t>(m_GeometryMaterials.size() - 1)});
            }
        }

        m_GeometryBuffers[frameIndex].MemCopy(geometry.data(), geometry.size() * sizeof(GeometryVertex));
        cmd.BindPipeline(m_QuadPipeline->GetPipeline());
        cmd.BindVertexBuffer(&m_GeometryBuffers[frameIndex].GetBuffer());
        cmd.BindDescriptorSets(m_QuadPipeline->GetPipelineLayout(), &m_GeometryTexturesDescriptorSet, 1);
        for (const auto& drawInfo : drawInfos)
        {
            const auto& material = m_GeometryMaterials[drawInfo.materialIdx];
            DebugGeometryPushConstant pushConstant{};
            pushConstant.viewProj = viewProj;
            pushConstant.textureIdx = material.textureIdx;
            pushConstant.color = material.color;
            cmd.PushConstant(m_QuadPipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstant);
            cmd.Draw(drawInfo.vertexCount, drawInfo.vertexOffset);
        }
    }
}

void FVkDebugDraw::Clear()
{
    m_Lines.clear();
    m_Points.clear();
    m_Quads.clear();
    m_Billboards.clear();
    m_GeometryMaterials.clear();
    m_GeometryDrawInfos.clear();
}
