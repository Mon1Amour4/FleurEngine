#pragma once

#include "Graphics.hpp"
#include "Image2D.h"

namespace Fleur::Graphics
{
class Skybox
{
public:
    Skybox(AssetID cubemapID)
        : m_Material{.albedo = cubemapID}
    {
    }

    inline const Fleur::Vec3* GetVertexData() const
    {
        return m_SkyboxVertices.data();
    }
    inline uint32_t GetVertexCount() const
    {
        return m_SkyboxVertices.size();
    }

    inline const Fleur::Graphics::FLMaterial* GetMaterial() const
    {
        return &m_Material;
    }


private:
    Fleur::Graphics::FLMaterial m_Material;
    static constexpr std::array<Fleur::Vec3, 36> m_SkyboxVertices = {
        // +X
        Fleur::Vec3(1, -1, -1),
        Fleur::Vec3(1, -1, 1),
        Fleur::Vec3(1, 1, 1),
        Fleur::Vec3(1, 1, 1),
        Fleur::Vec3(1, 1, -1),
        Fleur::Vec3(1, -1, -1),

        // -X
        Fleur::Vec3(-1, -1, 1),
        Fleur::Vec3(-1, -1, -1),
        Fleur::Vec3(-1, 1, -1),
        Fleur::Vec3(-1, 1, -1),
        Fleur::Vec3(-1, 1, 1),
        Fleur::Vec3(-1, -1, 1),

        // +Y
        Fleur::Vec3(-1, 1, -1),
        Fleur::Vec3(1, 1, -1),
        Fleur::Vec3(1, 1, 1),
        Fleur::Vec3(1, 1, 1),
        Fleur::Vec3(-1, 1, 1),
        Fleur::Vec3(-1, 1, -1),

        // -Y
        Fleur::Vec3(-1, -1, 1),
        Fleur::Vec3(1, -1, 1),
        Fleur::Vec3(1, -1, -1),
        Fleur::Vec3(1, -1, -1),
        Fleur::Vec3(-1, -1, -1),
        Fleur::Vec3(-1, -1, 1),

        // +Z
        Fleur::Vec3(-1, -1, 1),
        Fleur::Vec3(-1, 1, 1),
        Fleur::Vec3(1, 1, 1),
        Fleur::Vec3(1, 1, 1),
        Fleur::Vec3(1, -1, 1),
        Fleur::Vec3(-1, -1, 1),

        // -Z
        Fleur::Vec3(1, -1, -1),
        Fleur::Vec3(1, 1, -1),
        Fleur::Vec3(-1, 1, -1),
        Fleur::Vec3(-1, 1, -1),
        Fleur::Vec3(-1, -1, -1),
        Fleur::Vec3(1, -1, -1),
    };
};
}  // namespace Fleur::Graphics
