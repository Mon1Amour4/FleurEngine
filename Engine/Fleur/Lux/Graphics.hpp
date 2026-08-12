#pragma once

#include <Fleur/Math/Math.hpp>
#include <vector>

#define NODE_TRANSFORMS_MAX_CUP 1023

#include "glm/glm.hpp"

namespace Fleur::Graphics
{
using AssetID = uint32_t;

class BoundingBox
{
public:
    BoundingBox() = default;
    ~BoundingBox() = default;

    void UpdateBoundingBox(Fleur::Vec3 newMin, Fleur::Vec3 newMax)
    {
        min = Fleur::Math::min(min, newMin);
        max = Fleur::Math::max(max, newMax);

        center.x = (min.x + max.x) * 0.5;
        center.y = (min.y + max.y) * 0.5;
        center.z = (min.z + max.z) * 0.5;
    }

    inline Fleur::Vec3 GetMin() const
    {
        return min;
    }
    inline Fleur::Vec3 GetMax() const
    {
        return max;
    }
    inline Fleur::Vec3 GetCenter() const
    {
        return center;
    }

private:
    Fleur::Vec3 min{std::numeric_limits<float>::infinity()};
    Fleur::Vec3 max{-std::numeric_limits<float>::infinity()};
    Fleur::Vec3 center{0.f};
};

enum class FLAlphaMode
{
    FL_OPAQUE,
    FL_MASK,
    FL_BLEND
};

enum class LightSampling : uint32_t
{
    Default = 0,
    PCF_3x3,
    PCF_5x5,
    NoiseTexture,
};

// Engine-side material (source of truth, from glTF). Textures referenced by
// AssetID; each backend derives its own GPU material (resolved indices) from this.
struct FLMaterial
{
    Fleur::Vec4 baseColorFactor{1.f};
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
    Fleur::Vec3 Position;
    Fleur::Vec2 TexCoord;
    Fleur::Vec3 Normal;

    SVertexData(Fleur::Vec3 pos = Fleur::Vec3(0.0f), Fleur::Vec3 text_coord = Fleur::Vec3(0.0f), Fleur::Vec3 normal = Fleur::Vec3(0.0f))
        : Position(pos)
        , TexCoord(text_coord)
        , Normal(normal)
    {
    }
};
#pragma pack(pop)

struct SFLCameraData
{
    Fleur::Mat4 view;
    Fleur::Mat4 proj;
    Fleur::Vec3 cameraDir;
    Fleur::Vec3 cameraPos;
    float nearPlane;
    float farPlane;
};

struct DirectionalLightRenderData
{
    Fleur::Vec4 dirIntens{Fleur::Vec4(1, 0, 0, 1)};
    Fleur::Vec4 color{Fleur::Vec4(1, 1, 1, 1)};
    Fleur::Vec4 pos{Fleur::Vec4(0, 0, 0, 0)};
};

struct SFLPointLight
{
    Fleur::Vec3 pos;
    float radius;

    Fleur::Vec3 color;
    float intensity;
};

struct RenderFrameData
{
    SFLCameraData camera;
    DirectionalLightRenderData directionalLight;
    bool pointLightsDirty{false};
    std::vector<SFLPointLight> pointLights;
};

struct SFLSSBODescriptorBuffer
{
    Fleur::Mat4 modelTransform;
    Fleur::Mat4 nodeTransforms[NODE_TRANSFORMS_MAX_CUP];
};

struct SFLGeometryUBO
{
    Fleur::Mat4 view;
    Fleur::Mat4 proj;
};

enum ERenderStage : uint8_t
{
    STATIC_GEOMETRY,
    DYNAMIC_DRAW,
    GIZMO
};

}  // namespace Fleur::Graphics
