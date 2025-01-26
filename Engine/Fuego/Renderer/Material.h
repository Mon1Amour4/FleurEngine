#pragma once

#include "glm/common.hpp"

namespace Fuego::Renderer
{
class Shader;

class Material
{
public:
    Material(uint16_t albedo);
    virtual ~Material() = default;

    void Bind(Shader& shader);

private:
    // uint16_t albedo_texture;
};
}  // namespace Fuego::Renderer
