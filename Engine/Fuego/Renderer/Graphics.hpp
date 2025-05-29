#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace Fuego::Graphics
{

struct GraphicsResourceBase
{
    virtual ~GraphicsResourceBase() = default;

    virtual void PostCreate(const void* settings)
    {
        is_created = true;
    }

    virtual inline bool IsValid() const
    {
        return is_created;
    }
    virtual inline uint16_t Proirity()
    {
        return priority;
    }

protected:
    GraphicsResourceBase()
        : is_created(false)
        , priority(0)
    {
    }

    bool is_created;
    uint16_t priority;
};

enum GraphicsAPI
{
    OpenGL = 0,
    Vulkan = 1,
};

#pragma pack(push, 1)
struct VertexData
{
    glm::vec3 pos;
    glm::vec2 textcoord;
    glm::vec3 normal;

    VertexData(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 text_coord = glm::vec3(0.0f), glm::vec3 normal = glm::vec3(0.0f));
};
#pragma pack(pop)

struct Viewport
{
    float width = 0.0f;
    float height = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
};

enum TextureType
{
    ALBEDO = 0,
    DIFFUSE = 1,
    SPECULAR = 2
};

}  // namespace Fuego::Graphics