#pragma once

#include "Material.h"
#include "TextureOpenGL.h"

namespace Fuego::Graphics
{
class MaterialOpenGL : public Material
{
public:
    virtual ~MaterialOpenGL() override = default;

    inline const Texture2DOpenGL& GetAlbedoTexture() const
    {
        return *albedo_texture;
    }

    virtual void Use() const override;

private:
    const Texture2DOpenGL* albedo_texture;
    friend class Material;
    MaterialOpenGL(const TextureBase* albedo);
};
}  // namespace Fuego::Graphics
