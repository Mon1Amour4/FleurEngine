#pragma once

#include <vulkan/vulkan.h>

#include <Fleur/Math/Math.hpp>
#include <vector>

#include "FVkBuffer.h"
#include "FVkCommand.h"
#include "FVkDevice.h"
#include "FVkPipeline.h"
#include "FVkShader.h"
#include "FVkSwapchain.h"
#include "Graphics.hpp"

struct GeometryVertex
{
    Fleur::Math::vec3 pos;
    Fleur::Math::vec2 uv;
};

struct DebugGeometryPushConstant
{
    Fleur::Math::mat4 viewProj{1.f};
    Fleur::Math::ivec4 params{-1, 0, 0, 0};  // x = textureIdx, y = sample mode
    Fleur::Math::vec4 color{1.f};
};

struct SDebugVertex
{
    Fleur::Math::vec3 pos;
    Fleur::Math::vec4 color;
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
                VkDescriptorSetLayout geometryTexturesLayout,
                VkDescriptorSet geometryTexturesDescriptorSet,
                VkSampleCountFlagBits sampleCount,
                VkFormat depthFormat,
                uint32_t framesInFlight);
    // clang-format on

    // Per-frame: take this frame's accumulated geometry (from the frontend batch).
    // Accumulate primitives — the frontend forwards each DrawLine/DrawPoint here.
    void AddLine(Fleur::Math::vec3 a, Fleur::Math::vec3 b, Fleur::Math::vec3 color);
    void AddPoint(Fleur::Math::vec3 p, Fleur::Math::vec3 color, float size);
    void AddQuad(Fleur::Math::vec3 a, Fleur::Math::vec3 b, Fleur::Math::vec3 c, Fleur::Math::vec3 d, Fleur::Math::vec4 color);
    void AddQuad(Fleur::Math::vec3 a, Fleur::Math::vec3 b, Fleur::Math::vec3 c, Fleur::Math::vec3 d, uint32_t textureIdx);
    void AddBillboard(Fleur::Math::vec3 center, Fleur::Math::vec2 size, uint32_t textureIdx);
    void Frustum(const Fleur::Math::mat4& invViewProj, Fleur::Math::vec3 color);

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
    FVkPipeline* m_LinePipeline{nullptr};   // VK_PRIMITIVE_TOPOLOGY_LINE_LIST
    FVkPipeline* m_PointPipeline{nullptr};  // VK_PRIMITIVE_TOPOLOGY_POINT_LIST
    FVkPipeline* m_QuadPipeline{nullptr};   // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    VkDescriptorSetLayout m_GeometryTexturesLayout{VK_NULL_HANDLE};
    VkDescriptorSet m_GeometryTexturesDescriptorSet{VK_NULL_HANDLE};

    // Per-frame GPU buffers (x framesInFlight) so we never stomp data the GPU is
    std::vector<FVkBuffer> m_LineBuffers;
    std::vector<FVkBuffer> m_PointBuffers;

    struct PrimitiveMaterial
    {
        int32_t textureIdx{-1};
        int32_t textureSource{0};  // 0 = color, 1 = texture rgba, 2 = texture depth
        Fleur::Math::vec4 color{Fleur::Math::vec4(-1, -1, -1, -1)};
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
        Fleur::Math::vec3 center{};
        Fleur::Math::vec2 size{};
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
