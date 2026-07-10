#pragma once

#define NODE_TRANSFORMS_MAX_CUP 1023
#define POINT_LIGHTS_MAX_CUP 32

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include "glm/glm.hpp"

namespace Fleur::Graphics
{
using AssetID = uint32_t;

class BoundingBox
{
public:
    BoundingBox() = default;
    ~BoundingBox() = default;

    void UpdateBoundingBox(glm::vec3 newMin, glm::vec3 newMax)
    {
        min = glm::min(min, newMin);
        max = glm::max(max, newMax);

        center.x = (min.x + max.x) * 0.5;
        center.y = (min.y + max.y) * 0.5;
        center.z = (min.z + max.z) * 0.5;
    }

    inline glm::vec3 GetMin() const
    {
        return min;
    }
    inline glm::vec3 GetMax() const
    {
        return max;
    }
    inline glm::vec3 GetCenter() const
    {
        return center;
    }

private:
    glm::vec3 min{std::numeric_limits<float>::infinity()};
    glm::vec3 max{-std::numeric_limits<float>::infinity()};
    glm::vec3 center{0.f};
};

enum class FLAlphaMode
{
    FL_OPAQUE,
    FL_MASK,
    FL_BLEND
};

// Engine-side material (source of truth, from glTF). Textures referenced by
// AssetID; each backend derives its own GPU material (resolved indices) from this.
struct FLMaterial
{
    glm::vec4 baseColorFactor{1.f};
    AssetID albedo{0};
    AssetID normal{0};
    FLAlphaMode mode{FLAlphaMode::FL_OPAQUE};
    float alphaCutoff{0.f};
};
enum class EGraphicsAPI
{
    OpenGL = 0,
    Vulkan = 1,
};

#pragma pack(push, 1)
struct SVertexData
{
    glm::vec3 Position;
    glm::vec2 TexCoord;
    glm::vec3 Normal;

    SVertexData(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 text_coord = glm::vec3(0.0f), glm::vec3 normal = glm::vec3(0.0f))
        : Position(pos)
        , TexCoord(text_coord)
        , Normal(normal)
    {
    }
};
#pragma pack(pop)

struct SFLCameraData
{
    glm::vec3 cameraDir;
    glm::mat4 view;
    glm::mat4 proj;
};

struct DirectionalLightRenderData
{
    glm::vec4 dirIntens{glm::vec4(1, 0, 0, 1)};
    glm::vec4 color{glm::vec4(1, 1, 1, 1)};
    glm::vec4 pos{glm::vec4(0, 0, 0, 0)};
};

struct RenderFrameData
{
    SFLCameraData camera;
    DirectionalLightRenderData directionalLight;
};

struct SFLSSBODescriptorBuffer
{
    glm::mat4 modelTransform;
    glm::mat4 nodeTransforms[NODE_TRANSFORMS_MAX_CUP];
};

struct SFLGeometryUBO
{
    glm::mat4 view;
    glm::mat4 proj;
};

enum ERenderStage : uint8_t
{
    STATIC_GEOMETRY,
    DYNAMIC_DRAW,
    GIZMO
};

}  // namespace Fleur::Graphics
