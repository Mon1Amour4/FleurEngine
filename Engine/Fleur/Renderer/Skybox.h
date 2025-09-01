#pragma once

#include <span>

#include "Material.h"
#include "Texture.h"

namespace Fleur::Graphics
{
class Skybox
{
public:
    template <typename T, std::size_t N>
    Skybox(std::shared_ptr<Texture> cubemap, std::span<T, N> IN vertices)
        : m_Cubemap(cubemap)

    {
        FL_CORE_ASSERT(vertices.size() != 0, "");

        m_Vertices.assign(vertices.begin(), vertices.end());
        ShaderComponentContext ctx{};
        ctx.skybox_cubemap_text.second = cubemap.get();
        auto skyboxMaterial = Material::CreateMaterial(ctx);
        m_Material.reset(skyboxMaterial);
    }

    const Material* GetMaterial() const
    {
        return m_Material.get();
    }

    uint32_t GetVertexCount() const
    {
        return m_Vertices.size();
    };

    const float* Data() const
    {
        return m_Vertices.data();
    }

private:
    std::unique_ptr<Material> m_Material;
    std::shared_ptr<Texture> m_Cubemap;
    std::vector<float> m_Vertices;
};
}  // namespace Fleur::Graphics