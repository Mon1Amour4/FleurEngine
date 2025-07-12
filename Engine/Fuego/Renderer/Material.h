#pragma once

#include "ShaderObject.h"
#include "Texture.h"
#include "glm/common.hpp"

namespace Fuego::Graphics
{
class TextureBase;

struct ShaderComponentContext
{
    std::pair<std::string, Texture2D*> albedo_text{"albedo_texture", nullptr};
    std::pair<std::string, TextureCubemap*> skybox_cubemap_text{"skybox_cubemap", nullptr};
};


class FUEGO_API Material
{
public:
    template <typename T>
    static T* CreateMaterial(ShaderComponentContext& ctx)
    {
        return new T(ctx);
    }

    virtual ~Material() = default;

    virtual void SetParameters(ShaderObject& shader) const = 0;

    virtual void Use() const = 0;
    virtual inline std::string_view Name() const
    {
        return name;
    }

private:
    std::string name;
};

class SkyboxMaterial : public Material
{
    friend class Device;

public:
    virtual ~SkyboxMaterial() override {};

    virtual void Use() const override {};

    virtual void SetParameters(ShaderObject& shader) const override
    {
        shader.Set<TextureCubemap>(skybox_cubemap.first, *skybox_cubemap.second);
    }

    SkyboxMaterial(ShaderComponentContext& ctx)
        : Material()
    {
        skybox_cubemap = std::make_pair(std::move(ctx.skybox_cubemap_text.first), ctx.skybox_cubemap_text.second);
    }

private:
    std::pair<std::string, TextureCubemap*> skybox_cubemap;
};

class OpaqueMaterial : public Material
{
public:
    virtual ~OpaqueMaterial() override {};
    virtual void Use() const override {};
    virtual void SetParameters(ShaderObject& shader) const override
    {
        shader.Set(albedo_text.first, albedo_text.second);
    }

    OpaqueMaterial(ShaderComponentContext& ctx)
        : Material()
    {
        albedo_text = std::make_pair(std::move(ctx.albedo_text.first), ctx.albedo_text.second);
    }

private:
    std::pair<std::string, Texture2D*> albedo_text;
};

}  // namespace Fuego::Graphics
