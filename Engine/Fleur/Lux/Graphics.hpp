#pragma once

#define NODE_TRANSFORMS_MAX_CUP 1023

#include "glm/glm.hpp"
namespace Fleur::Graphics
{
using AssetID = uint32_t;

struct SAABB
{
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
