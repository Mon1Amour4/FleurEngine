#pragma once

#include "ShaderObject.h"
#include "Texture.h"
#include "glm/common.hpp"

namespace Fleur::Graphics
{
class TextureBase;

struct ShaderComponentContext
{
    std::pair<std::string, const Texture*> albedo_text{"material.albedo_texture", nullptr};
    std::pair<std::string, Texture*> skybox_cubemap_text{"material.skybox_cubemap", nullptr};
};


class FLEUR_API Material
{
public:
    static Material* CreateMaterial(ShaderComponentContext& ctx)
    {
        return new Material(ctx);
    }

    Material(ShaderComponentContext& ctx)
    {
        if (ctx.albedo_text.second)
            m_Context.albedo_text = std::move(ctx.albedo_text);
        if (ctx.skybox_cubemap_text.second)
            m_Context.skybox_cubemap_text = std::move(ctx.skybox_cubemap_text);
    }
    virtual ~Material() = default;

    inline std::string_view Name() const
    {
        return m_Name;
    }

    const ShaderComponentContext& GetShaderContext() const
    {
        return m_Context;
    }

private:
    std::string m_Name;
    ShaderComponentContext m_Context;
};

}  // namespace Fleur::Graphics
