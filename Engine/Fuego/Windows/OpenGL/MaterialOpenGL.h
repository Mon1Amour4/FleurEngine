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
    MaterialOpenGL(uint32_t albedo);
    uint32_t albedo_texture;
};
}  // namespace Fuego::Renderer
