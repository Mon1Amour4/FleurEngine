#pragma once

#include <Fleur/Math/Math.hpp>
#include <vector>

#include "AssetHandle.h"
#include "LightingSystem.h"
#include "Lux/Lux.h"  // Lux::Renderer, AssetID

namespace Fleur::Graphics
{
class Camera;
}

namespace Fleur
{
struct SceneInstance
{
    Fleur::AssetHandle model;
    Mat4 transform;
};

// Owns the world's instances + the camera. Submits draws to Lux each frame.
// Hardcoded contents for now (no serialization / ECS yet).
//
// Layering: Scene -> Lux (Draw), Scene -> AssetsManager (requests model loads).
// Scene does no file IO of its own and never touches the GPU directly.
class Scene : public IUpdatable
{
public:
    explicit Scene(Fleur::Graphics::LightingSystem* lightingSystem);
    ~Scene();

    void Init();  // hardcode instances + camera
    void OnUpdate(float dtTime) override;

    void Submit(Lux::Renderer& renderer);  // per-frame draw submission

    [[nodiscard]] Fleur::Graphics::RenderFrameData GetFrameData();

private:
    Fleur::Graphics::LightingSystem* m_LightingSystem{nullptr};
    Fleur::Graphics::DirectionalLightHandle m_DirectionalLightHandle;
    std::vector<Fleur::Graphics::PointLightHandle> m_PointLightHandles;

    std::vector<SceneInstance> m_Instances;
    Graphics::Camera* m_Camera{nullptr};

    uint32_t m_FloorTextureIdx;
    uint32_t m_SunTextureIdx;
};
}  // namespace Fleur
