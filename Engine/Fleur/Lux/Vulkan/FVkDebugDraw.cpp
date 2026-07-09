#include "FVkDebugDraw.h"

FVkDebugDraw::FVkDebugDraw() = default;

FVkDebugDraw::~FVkDebugDraw()
{
    delete m_LinePipeline;
    delete m_PointPipeline;

    m_LineBuffers.clear();
    m_LineBuffers.shrink_to_fit();

    m_PointBuffers.clear();
    m_PointBuffers.shrink_to_fit();
}

void FVkDebugDraw::Create(const FVkDevice* device, const FVkSwapchain* swapchain, vk::FVkShader* debugShader, VkSampleCountFlagBits sampleCount,
                          VkFormat depthFormat, uint32_t framesInFlight)
{
    m_Device = device->GetLogicalDevice();
    m_PhysicalDevice = device->GetPhysicalDevice();
    m_DebugShader = debugShader;
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
    FVkBuffer& lineBuffer = m_LineBuffers.emplace_back();
    lineBuffer.Init(m_Device, m_PhysicalDevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, kVertexBufferSize, kVertexBufferStride);

    //   m_PointBuffers likewise.
    FVkBuffer& pointBuffer = m_PointBuffers.emplace_back();
    pointBuffer.Init(m_Device, m_PhysicalDevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, kVertexBufferSize, kVertexBufferStride);

    createPipelines();

    m_Initialized = true;
}

void FVkDebugDraw::createPipelines()
{
    if (m_DebugShader)
    {
        if (!m_DebugShader->isInitialized())
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
        m_LinePipeline = m_DebugShader->GetPipeline(linePipelineInfo, descriptorSetLayouts);
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

        m_PointPipeline = m_DebugShader->GetPipeline(pointPipelineInfo, descriptorSetLayouts);
        assert(m_PointPipeline);
    }

    // TODO: build the two pipelines from m_DebugShader (shared pos + color shader).
    //   common state: vertex input { vec3 pos @0, uint color (R8G8B8A8_UNORM) @12 },
    //                 depthTest = true, depthWrite = false, viewProj via push-constant.
    //   line  pipeline: topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,  lineWidth = 1.0
    //   point pipeline: topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST  (VS writes gl_PointSize)
    //
    //   m_LinePipeline  = m_DebugShader->GetPipeline(lineInfo);
    //   m_PointPipeline = m_DebugShader->GetPipeline(pointInfo);
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

void FVkDebugDraw::Record(FVkCommandBuffer& cmd, Fleur::Graphics::SFLCameraData& cameraData, uint32_t frameIndex)
{
    glm::mat4 viewProj = cameraData.proj * cameraData.view;

    if (!m_Lines.empty())
    {
        m_LineBuffers[0].MemCopy(m_Lines.data(), m_Lines.size() * sizeof(SDebugVertex));
        cmd.BindPipeline(m_LinePipeline->GetPipeline());
        cmd.BindVertexBuffer(&m_LineBuffers[0].GetBuffer());
        cmd.PushConstant(m_LinePipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, viewProj);
        cmd.Draw(static_cast<uint32_t>(m_Lines.size()), 0);  // LINE_LIST: all lines, one draw
    }

    if (!m_Points.empty())
    {
        m_PointBuffers[0].MemCopy(m_Points.data(), m_Points.size() * sizeof(SDebugVertex));
        cmd.BindPipeline(m_PointPipeline->GetPipeline());
        cmd.BindVertexBuffer(&m_PointBuffers[0].GetBuffer());
        cmd.PushConstant(m_LinePipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, viewProj);
        cmd.Draw(static_cast<uint32_t>(m_Points.size()), 0);
    }
}

void FVkDebugDraw::Clear()
{
    m_Lines.clear();
    m_Points.clear();
}
