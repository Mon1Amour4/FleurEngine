#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "AssetHandle.h"
#include "DirectionalLight.h"
#include "Lux/Lux.h"  // Lux::Renderer, AssetID
#include "OmniLight.h"

namespace Fleur::Graphics
{
class Camera;
}

namespace Fleur
{
struct SceneInstance
{
    Fleur::AssetHandle model;
    glm::mat4 transform;
};

// Owns the world's instances + the camera. Submits draws to Lux each frame.
// Hardcoded contents for now (no serialization / ECS yet).
//
// Layering: Scene -> Lux (Draw), Scene -> AssetsManager (requests model loads).
// Scene does no file IO of its own and never touches the GPU directly.
class Scene : public IUpdatable
{
public:
    Scene();
    ~Scene();

    void Init();  // hardcode instances + camera
    void OnUpdate(float dtTime) override;

    void Submit(Lux::Renderer& renderer);  // per-frame draw submission

    [[nodiscard]] Fleur::Graphics::RenderFrameData GetFrameData() const;

private:
    Fleur::Graphics::DirectionalLight m_DirectionalLight;
    std::vector<Fleur::Graphics::OmniLight> m_OmniLights;

    std::vector<SceneInstance> m_Instances;
    Graphics::Camera* m_Camera{nullptr};
};
}  // namespace Fleur
