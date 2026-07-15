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

struct OverlayVertex
{
    Fleur::Math::vec3 pos;
    Fleur::Math::vec2 uv;
};

struct OverlayPushConstant
{
    Fleur::Math::ivec4 params{-1, 0, 0, 0};  // x = textureIdx, y = sample mode
    Fleur::Math::vec4 color{1.f};
};

class FVkOverlayPass
{
public:
    FVkOverlayPass() = default;
    ~FVkOverlayPass();

    void Create(const FVkDevice* device, const FVkSwapchain* swapchain, VkDescriptorSetLayout texturesLayout, VkDescriptorSet texturesDescriptorSet,
                uint32_t shadowMapTextureSlot, VkSampleCountFlagBits sampleCount, VkFormat depthFormat, uint32_t framesInFlight);
    void SetShader(vk::FVkShader* overlayShader);

    void AddQuad(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, Fleur::Math::vec2 d, Fleur::Math::vec4 color);
    void AddQuad(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, Fleur::Math::vec2 d, uint32_t textureIdx);
    void AddTriangle(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, Fleur::Math::vec4 color);
    void AddTriangle(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, uint32_t textureIdx);
    void AddShadowMapQuad(Fleur::Math::vec2 min, Fleur::Math::vec2 max);

    void Record(FVkCommandBuffer& cmd, uint32_t frameIndex);
    void Clear();

    bool IsInitialized() const
    {
        return m_Initialized;
    }

private:
    struct Material
    {
        int32_t textureIdx{-1};
        int32_t textureSource{0};  // 0 = color, 1 = texture rgba, 2 = texture depth
        Fleur::Math::vec4 color{Fleur::Math::vec4(-1, -1, -1, -1)};
    };

    struct DrawInfo
    {
        uint32_t vertexCount;
        uint32_t vertexOffset;
        uint32_t materialIdx;
    };

    bool m_Initialized{false};
    VkDevice m_Device{nullptr};
    VkPhysicalDevice m_PhysicalDevice{nullptr};

    vk::FVkShader* m_Shader{nullptr};
    FVkPipeline* m_Pipeline{nullptr};
    VkDescriptorSetLayout m_TexturesLayout{VK_NULL_HANDLE};
    VkDescriptorSet m_TexturesDescriptorSet{VK_NULL_HANDLE};
    uint32_t m_ShadowMapTextureSlot{0};

    std::vector<FVkBuffer> m_Buffers;
    std::vector<OverlayVertex> m_Geometry;
    std::vector<Material> m_Materials;
    std::vector<DrawInfo> m_DrawInfos;

    VkFormat m_ColorFormat{VK_FORMAT_UNDEFINED};
    VkFormat m_DepthFormat{VK_FORMAT_UNDEFINED};
    VkSampleCountFlagBits m_SampleCount{VK_SAMPLE_COUNT_1_BIT};

    static constexpr uint32_t kMaxVertsPerFrame = 256u * 1024u;
};
