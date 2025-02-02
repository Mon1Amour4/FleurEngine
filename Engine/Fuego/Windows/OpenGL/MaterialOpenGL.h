#pragma once

#include "Material.h"
#include "TextureOpenGL.h"

namespace Fuego::Renderer
{
class MaterialOpenGL : public Material
{
public:
    virtual ~MaterialOpenGL() override = default;

    virtual void Upload(ShaderObject& shader) override;
    inline const TextureOpenGL& GetAlbedoTexture() const
    {
        return *albedo_texture;
    }

private:
    TextureOpenGL* albedo_texture;
    friend class Material;
    MaterialOpenGL(Texture* albedo);
};
}  // namespace Fuego::Renderer
