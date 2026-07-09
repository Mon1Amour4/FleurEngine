#pragma once

#include <glm/glm.hpp>

#include "IRenderer.hpp"  // IRenderer + DTOs
#include "Services/ServiceInterfaces.hpp"  // Service<>

namespace Fleur::Graphics
{
class Shader;
}

namespace Lux
{
using AssetID = Fleur::Graphics::AssetID;

// What the scene hands the renderer for a frame's view.
struct CameraView
{
    glm::mat4 view{1.0f};
    glm::mat4 proj{1.0f};
    glm::vec3 dir{0.0f};  // camera forward
};

// Frontend renderer service. Owns the graphics backend (IRenderer), drives the frame.
//   SetBackend: (re)creates the backend + passes + skybox (runtime-selected; OnInit is a no-op).
//   Application: drives the frame loop (BeginFrame/Draw via Scene/EndFrame) + Register/Upload.
class Renderer : public Fleur::Service<Renderer>
{
public:
    friend struct Fleur::Service<Renderer>;

    Renderer() = default;
    ~Renderer();

    // Resource lifetime (retained). Forward on asset load, inverse on evict.
    void Register(const Fleur::Graphics::SFLModelRegisterInfo& info);

    void Unregister(Fleur::Graphics::AssetID model);
    void UploadTextures(Fleur::Graphics::SFLImageViewInfo* info);
    void RemoveTexture(AssetID texture);

    // Frame (immediate-mode).
    void BeginFrame(const CameraView& camera);
    void Draw(AssetID model, const glm::mat4& transform);
    void EndFrame();

    // Window / engine.
    void StartResize();
    void EndResize(Fleur::SRect& rect);
    void SetSkybox(AssetID id);
    void SetVSync(bool active) { m_Vsync = active; }
    bool IsVSync() const { return m_Vsync; }

    // Runtime backend selection / live switching (all backends are compiled in).
    void SetBackend(Fleur::Graphics::EGraphicsAPI api);
    Fleur::Graphics::EGraphicsAPI GetBackendApi() const { return m_Api; }

protected:
    void OnInit();
    void OnShutdown();

private:
    Fleur::Graphics::SFLShaderInfo shaderInfo(Fleur::Graphics::Shader* shader);
    void initBackend();

    Fleur::Graphics::IRenderer* m_Backend{nullptr};
    Fleur::Graphics::EGraphicsAPI m_Api{Fleur::Graphics::EGraphicsAPI::Vulkan};
    bool m_Vsync{true};
};
}  // namespace Lux
