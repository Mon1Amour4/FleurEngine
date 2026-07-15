#pragma once

#include <Fleur/Math/Math.hpp>

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

    void UpdateBoundingBox(Fleur::Math::vec3 newMin, Fleur::Math::vec3 newMax)
    {
        min = Fleur::Math::min(min, newMin);
        max = Fleur::Math::max(max, newMax);

        center.x = (min.x + max.x) * 0.5;
        center.y = (min.y + max.y) * 0.5;
        center.z = (min.z + max.z) * 0.5;
    }

    inline Fleur::Math::vec3 GetMin() const
    {
        return min;
    }
    inline Fleur::Math::vec3 GetMax() const
    {
        return max;
    }
    inline Fleur::Math::vec3 GetCenter() const
    {
        return center;
    }

private:
    Fleur::Math::vec3 min{std::numeric_limits<float>::infinity()};
    Fleur::Math::vec3 max{-std::numeric_limits<float>::infinity()};
    Fleur::Math::vec3 center{0.f};
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
    Fleur::Math::vec4 baseColorFactor{1.f};
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
    Fleur::Math::vec3 Position;
    Fleur::Math::vec2 TexCoord;
    Fleur::Math::vec3 Normal;

    SVertexData(Fleur::Math::vec3 pos = Fleur::Math::vec3(0.0f), Fleur::Math::vec3 text_coord = Fleur::Math::vec3(0.0f), Fleur::Math::vec3 normal = Fleur::Math::vec3(0.0f))
        : Position(pos)
        , TexCoord(text_coord)
        , Normal(normal)
    {
    }
};
#pragma pack(pop)

struct SFLCameraData
{
    Fleur::Math::vec3 cameraDir;
    Fleur::Math::mat4 view;
    Fleur::Math::mat4 proj;
};

struct DirectionalLightRenderData
{
    Fleur::Math::vec4 dirIntens{Fleur::Math::vec4(1, 0, 0, 1)};
    Fleur::Math::vec4 color{Fleur::Math::vec4(1, 1, 1, 1)};
    Fleur::Math::vec4 pos{Fleur::Math::vec4(0, 0, 0, 0)};
};

struct RenderFrameData
{
    SFLCameraData camera;
    DirectionalLightRenderData directionalLight;
};

struct SFLSSBODescriptorBuffer
{
    Fleur::Math::mat4 modelTransform;
    Fleur::Math::mat4 nodeTransforms[NODE_TRANSFORMS_MAX_CUP];
};

struct SFLGeometryUBO
{
    Fleur::Math::mat4 view;
    Fleur::Math::mat4 proj;
};

enum ERenderStage : uint8_t
{
    STATIC_GEOMETRY,
    DYNAMIC_DRAW,
    GIZMO
};

}  // namespace Fleur::Graphics
