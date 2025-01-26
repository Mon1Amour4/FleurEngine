#pragma once

#include "Material.h"

namespace Fuego::Renderer
{
class MaterialOpenGL : public Material
{
    MaterialOpenGL(uint16_t albedo);
    virtual ~MaterialOpenGL() override = default;

    virtual void Bind(Shader& shader) override;

private:
    uint16_t albedo_texture;
};
}  // namespace Fuego::Renderer