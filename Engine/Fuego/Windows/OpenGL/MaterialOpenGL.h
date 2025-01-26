#pragma once

#include "Material.h"

namespace Fuego::Renderer
{
class MaterialOpenGL : public Material
{
    virtual ~MaterialOpenGL() override = default;

    virtual void Bind(Shader& shader) override;

private:
    friend class Material;
    MaterialOpenGL(uint16_t albedo);
    uint16_t albedo_texture;
};
}  // namespace Fuego::Renderer
