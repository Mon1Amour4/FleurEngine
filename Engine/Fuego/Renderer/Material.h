#pragma once

#include "glm/common.hpp"

namespace Fuego::Renderer
{
class Shader;

class Material
{
public:
    static Material* CreateMaterial(uint32_t albedo);

    virtual ~Material() = default;

    virtual void Bind(Shader& shader) = 0;

private:
};
}  // namespace Fuego::Renderer
