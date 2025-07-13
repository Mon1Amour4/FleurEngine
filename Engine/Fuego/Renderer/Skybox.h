#pragma once

#include <span>

#include "Material.h"
#include "Texture.h"

namespace Fuego::Graphics
{
class Skybox
{
public:
    template <typename T, std::size_t N>
    Skybox(std::shared_ptr<Texture> cm, std::span<T, N> in_vertices)
        : cubemap(cm)

    {
        FU_CORE_ASSERT(in_vertices.size() != 0, "");

        vertices.assign(in_vertices.begin(), in_vertices.end());
        ShaderComponentContext ctx{};
        ctx.skybox_cubemap_text.second = cm.get();
        auto skybox_material = Material::CreateMaterial<SkyboxMaterial>(ctx);
        material.reset(skybox_material);
    }

    const Material* GetMaterial() const
    {
        return material.get();
    }

    uint32_t GetVertexCount() const
    {
        return vertices.size();
    };

private:
    std::unique_ptr<SkyboxMaterial> material;
    std::shared_ptr<Texture> cubemap;
    std::vector<float> vertices;
};
}  // namespace Fuego::Graphics