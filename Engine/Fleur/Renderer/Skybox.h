#pragma once

#include "Graphics.hpp"
#include "Image2D.h"

namespace Fleur::Graphics
{
class Skybox
{
public:
    Skybox(AssetID cubemapID)
        : m_CubemapID(cubemapID)
    {
    }

    inline glm::vec3& GetVertexData()
    {
    }


private:
    AssetID m_CubemapID;
    static constexpr std::array<glm::vec3, 36> SkyboxVertices = {
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