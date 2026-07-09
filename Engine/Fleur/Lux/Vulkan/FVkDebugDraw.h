#pragma once

#include <vulkan/vulkan.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/vec3.hpp>
#include <vector>

#include "FVkBuffer.h"
#include "FVkCommand.h"
#include "FVkDevice.h"
#include "FVkPipeline.h"
#include "FVkShader.h"
#include "FVkSwapchain.h"
#include "Graphics.hpp"

// GPU debug vertex. Color is packed RGBA8 (Color -> u32 conversion happens on the
// frontend, so this type never depends on the engine Color).
// NOTE: the frontend accumulator needs the same layout — once wired, move this to
// a shared header (RenderViews.hpp) instead of duplicating it.
struct SDebugVertex
{
    glm::vec3 pos;
    glm::vec4 color;
};

// Self-contained debug-geometry renderer (lines + points). Owns its own pipelines,
// shader and per-frame vertex buffers — mirrors FVkSkybox so vk::backend::impl
// stays uncluttered: it just holds a pointer and calls Submit/Record.
class FVkDebugDraw
{
public:
    FVkDebugDraw();
    ~FVkDebugDraw();

    // clang-format off
    void Create(const FVkDevice* device,
                const FVkSwapchain* swapchain,
                vk::FVkShader* debugShader,
                VkSampleCountFlagBits sampleCount,
                VkFormat depthFormat,
                uint32_t framesInFlight);
    // clang-format on

    // Per-frame: take this frame's accumulated geometry (from the frontend batch).
    // Accumulate primitives — the frontend forwards each DrawLine/DrawPoint here.
    void AddLine(glm::vec3 a, glm::vec3 b, glm::vec3 color);
    void AddPoint(glm::vec3 p, glm::vec3 color, float size);

    // Per-frame: upload to the frame's buffer + record the debug pass (call inside
    // the active dynamic-rendering pass, after geometry/skybox).
    void Record(FVkCommandBuffer& cmd, Fleur::Graphics::SFLCameraData& cameraData, uint32_t frameIndex);

    void Clear();

    inline bool IsInitialized() const
    {
        return m_Initialized;
    }

private:
    bool m_Initialized{false};
    // Builds the line (LINE_LIST) + point (POINT_LIST) pipelines from m_DebugShader.
    void createPipelines();

    VkDevice m_Device{nullptr};
    VkPhysicalDevice m_PhysicalDevice{nullptr};

    vk::FVkShader* m_DebugShader{nullptr};  // shared pos + color
    FVkPipeline* m_LinePipeline{nullptr};   // VK_PRIMITIVE_TOPOLOGY_LINE_LIST
    FVkPipeline* m_PointPipeline{nullptr};  // VK_PRIMITIVE_TOPOLOGY_POINT_LIST

    // Per-frame GPU buffers (x framesInFlight) so we never stomp data the GPU is
    // still reading from a prior frame.
    std::vector<FVkBuffer> m_LineBuffers;
    std::vector<FVkBuffer> m_PointBuffers;

    // CPU accumulation for the current frame (filled by Submit, cleared by Clear).
    std::vector<SDebugVertex> m_Lines;
    std::vector<SDebugVertex> m_Points;

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
