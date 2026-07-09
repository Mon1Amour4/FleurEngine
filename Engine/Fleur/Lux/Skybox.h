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

    inline const glm::vec3* GetVertexData() const
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
    static constexpr std::array<glm::vec3, 36> m_SkyboxVertices = {
        // +X
        glm::vec3(1, -1, -1),
        glm::vec3(1, -1, 1),
        glm::vec3(1, 1, 1),
        glm::vec3(1, 1, 1),
        glm::vec3(1, 1, -1),
        glm::vec3(1, -1, -1),

        // -X
        glm::vec3(-1, -1, 1),
        glm::vec3(-1, -1, -1),
        glm::vec3(-1, 1, -1),
        glm::vec3(-1, 1, -1),
        glm::vec3(-1, 1, 1),
        glm::vec3(-1, -1, 1),

        // +Y
        glm::vec3(-1, 1, -1),
        glm::vec3(1, 1, -1),
        glm::vec3(1, 1, 1),
        glm::vec3(1, 1, 1),
        glm::vec3(-1, 1, 1),
        glm::vec3(-1, 1, -1),

        // -Y
        glm::vec3(-1, -1, 1),
        glm::vec3(1, -1, 1),
        glm::vec3(1, -1, -1),
        glm::vec3(1, -1, -1),
        glm::vec3(-1, -1, -1),
        glm::vec3(-1, -1, 1),

        // +Z
        glm::vec3(-1, -1, 1),
        glm::vec3(-1, 1, 1),
        glm::vec3(1, 1, 1),
        glm::vec3(1, 1, 1),
        glm::vec3(1, -1, 1),
        glm::vec3(-1, -1, 1),

        // -Z
        glm::vec3(1, -1, -1),
        glm::vec3(1, 1, -1),
        glm::vec3(-1, 1, -1),
        glm::vec3(-1, 1, -1),
        glm::vec3(-1, -1, -1),
        glm::vec3(1, -1, -1),
    };
};
}  // namespace Fleur::Graphics