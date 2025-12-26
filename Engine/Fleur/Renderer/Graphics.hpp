#pragma once

namespace Fleur::Graphics
{

enum EGraphicsAPI
{
    OpenGL = 0,
    Vulkan = 1,
};

#pragma pack(push, 1)
struct VertexData
{
    glm::vec3 Position;
    glm::vec2 TexCoord;
    glm::vec3 Normal;

    VertexData(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 text_coord = glm::vec3(0.0f), glm::vec3 normal = glm::vec3(0.0f));
};
#pragma pack(pop)

struct Viewport
{
    uint32_t width = 0.0f;
    uint32_t height = 0.0f;
    uint32_t x = 0.0f;
    uint32_t y = 0.0f;
};

enum class ETextureUsage
{
    ALBEDO = 0,
    DIFFUSE = 1,
    SPECULAR = 2
};

enum ERenderStage : uint8_t
{
    STATIC_GEOMETRY,
    DYNAMIC_DRAW,
    GIZMO
};

}  // namespace Fleur::Graphics
