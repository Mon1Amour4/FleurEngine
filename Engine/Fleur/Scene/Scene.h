#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "Renderer/Lux.h"  // Lux::Renderer, Lux::CameraView, AssetID

namespace Fleur::Graphics
{
class Camera;
}

namespace Fleur
{
struct SceneInstance
{
    Graphics::AssetID model;
    glm::mat4 transform;
};

// Owns the world's instances + the camera. Submits draws to Lux each frame.
// Hardcoded contents for now (no serialization / ECS yet).
//
// Layering: Scene -> Lux (Draw), Scene -> AssetsManager (models). Scene never
// loads assets and never touches the GPU directly.
class Scene
{
public:
    Scene();
    ~Scene();

    void Init();  // hardcode instances + camera
    void Update(float dtTime);

    void Submit(Lux::Renderer& renderer);  // per-frame draw submission
    Lux::CameraView GetCamera() const;

private:
    std::vector<SceneInstance> m_Instances;
    Graphics::Camera* m_Camera{nullptr};
};
}  // namespace Fleur
