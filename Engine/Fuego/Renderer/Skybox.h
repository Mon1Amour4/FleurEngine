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
    Skybox(std::shared_ptr<TextureCubemap> cm, std::span<T, N> in_vertices)
        : cubemap(cm)

    {
        FU_CORE_ASSERT(in_vertices.size() != 0, "");

        vertices.assign(in_vertices.begin(), in_vertices.end());
        material.reset(Material::CreateMaterial(cm.get()));
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
    std::unique_ptr<Material> material;
    std::shared_ptr<TextureCubemap> cubemap;
    std::vector<float> vertices;
};
}  // namespace Fuego::Graphics