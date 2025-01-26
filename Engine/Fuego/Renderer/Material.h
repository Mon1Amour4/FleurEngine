#pragma once

#include "glm/common.hpp"

namespace Fuego::Renderer
{
class Material
{
public:
    Material(uint16_t albedo);
    virtual ~Material() = default;

private:
    uint16_t albedo_texture;
};
}  // namespace Fuego::Renderer
