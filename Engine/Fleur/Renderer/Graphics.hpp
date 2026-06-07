#pragma once

#include "glm/glm.hpp"

namespace Fleur::Graphics
{
enum FLAlphaMode
{
    FL_OPAQUE,
    FL_MASK,
    FL_BLEND
};
enum EGraphicsAPI
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

struct SFLGeometryUBO
{
    glm::mat4 model;
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
