#pragma once

#include <vulkan/vulkan.h>

#include <Fleur/Math/Math.hpp>
#include <memory>
#include <vector>

#include "FVkBuffer.h"
#include "FVkCommand.h"
#include "FVkDevice.h"
#include "FVkPipeline.h"
#include "FVkPipelineCache.h"
#include "FVkPipelineLayout.h"
#include "FVkShader.h"
#include "FVkSwapchain.h"
#include "Graphics.hpp"

struct GeometryVertex
{
    Fleur::Vec3 pos;
    Fleur::Vec2 uv;
};

struct DebugGeometryPushConstant
{
    Fleur::Mat4 viewProj{1.f};
    Fleur::IVec4 params{-1, 0, 0, 0};  // x = textureIdx, y = sample mode
    Fleur::Vec4 color{1.f};
};

struct SDebugVertex
{
    Fleur::Vec3 pos;
    Fleur::Vec4 color;
};

// Self-contained debug-geometry renderer (lines + points). Owns its own pipelines,
// shader and per-frame vertex buffers — mirrors FVkSkybox so vk::backend::impl
// stays uncluttered: it just holds a pointer and calls Submit/Record.
class FVkDebugDraw
{
public:
    FVkDebugDraw() = default;
    ~FVkDebugDraw();

    // clang-format off
    void Create(const FVkDevice* device,
                const FVkSwapchain* swapchain,
                vk::FVkShader* primitivesShader,
                vk::FVkShader* geometryShader,
                VkDescriptorSet geometryTexturesDescriptorSet,
                VkSampleCountFlagBits sampleCount,
                VkFormat depthFormat,
                uint32_t framesInFlight);
    // clang-format on

    // Per-frame: take this frame's accumulated geometry (from the frontend batch).
    // Accumulate primitives — the frontend forwards each DrawLine/DrawPoint here.
    void AddLine(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 color);
    void AddPoint(Fleur::Vec3 p, Fleur::Vec3 color, float size);
    void AddQuad(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 c, Fleur::Vec3 d, Fleur::Vec4 color);
    void AddQuad(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 c, Fleur::Vec3 d, uint32_t textureIdx);
    void AddBillboard(Fleur::Vec3 center, Fleur::Vec2 size, uint32_t textureIdx);
    void Frustum(const Fleur::Mat4& invViewProj, Fleur::Vec3 color);

    void RecordWorld(FVkCommandBuffer& cmd, const Fleur::Graphics::SFLCameraData& cameraData, uint32_t frameIndex);

    void Clear();

    inline bool IsInitialized() const
    {
        return m_Initialized;
    }

private:
    bool m_Initialized{false};
    // Builds the line/point pipelines from m_PrimitivesShader and the triangle pipeline
    // from m_GeometryShader.
    void createPipelines();

    VkDevice m_Device{nullptr};
    VkPhysicalDevice m_PhysicalDevice{nullptr};

    vk::FVkShader* m_PrimitivesShader{nullptr};
    vk::FVkShader* m_GeometryShader{nullptr};
    FVkPipeline* m_LinePipeline{nullptr};   // borrowed from m_PipelineCache
    FVkPipeline* m_PointPipeline{nullptr};  // borrowed from m_PipelineCache
    FVkPipeline* m_QuadPipeline{nullptr};   // borrowed from m_PipelineCache
    std::shared_ptr<FVkPipelineLayout> m_PrimitivesPipelineLayout;
    std::shared_ptr<FVkPipelineLayout> m_GeometryPipelineLayout;
    FVkPipelineCache m_PipelineCache;
    VkDescriptorSet m_GeometryTexturesDescriptorSet{VK_NULL_HANDLE};

    // Per-frame GPU buffers (x framesInFlight) so we never stomp data the GPU is
    std::vector<FVkBuffer> m_LineBuffers;
    std::vector<FVkBuffer> m_PointBuffers;

    struct PrimitiveMaterial
    {
        int32_t textureIdx{-1};
        int32_t textureSource{0};  // 0 = color, 1 = texture rgba, 2 = texture depth
        Fleur::Vec4 color{Fleur::Vec4(-1, -1, -1, -1)};
    };
    struct PrimitiveGeometryDrawInfo
    {
        uint32_t vertexCount;
        uint32_t vertexOffset;
        uint32_t materialIdx;
    };
    std::vector<PrimitiveMaterial> m_GeometryMaterials;
    std::vector<PrimitiveGeometryDrawInfo> m_GeometryDrawInfos;
    std::vector<FVkBuffer> m_GeometryBuffers;

    struct BillboardDrawInfo
    {
        Fleur::Vec3 center{};
        Fleur::Vec2 size{};
        uint32_t textureIdx{};
    };
    std::vector<BillboardDrawInfo> m_Billboards;

    // CPU accumulation for the current frame (filled by Submit, cleared by Clear).
    std::vector<SDebugVertex> m_Lines;
    std::vector<SDebugVertex> m_Points;
    std::vector<GeometryVertex> m_Quads;

    VkFormat m_ColorFormat{VK_FORMAT_UNDEFINED};
    VkFormat m_DepthFormat{VK_FORMAT_UNDEFINED};
    VkSampleCountFlagBits m_SampleCount{VK_SAMPLE_COUNT_1_BIT};
    VkExtent2D m_Extent{0, 0};
    uint32_t m_FramesInFlight{0};

    // Per-frame buffer capacity. TODO: tune. 256k verts * 16B ~= 4 MB per frame.
    static constexpr uint32_t kMaxVertsPerFrame = 256u * 1024u;
    static constexpr uint32_t kVertexBufferStride = sizeof(SDebugVertex);
    static constexpr uint32_t kVertexBufferSize = kMaxVertsPerFrame * kVertexBufferStride;
};
