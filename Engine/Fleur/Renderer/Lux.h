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
//   OnInit: create backend + passes + skybox.
//   Application: drives the frame loop (BeginFrame/Draw via Scene/EndFrame) + Register/Upload.
class Renderer : public Fleur::Service<Renderer>
{
public:
    friend struct Fleur::Service<Renderer>;

    Renderer() = default;
    ~Renderer();

    // Resource lifetime (retained). Forward on asset load, inverse on evict.
    void Register(AssetID model, const Fleur::Graphics::SVertexData* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount,
                  const Fleur::Graphics::FLDrawItem* primitives, uint32_t primitiveCount);
    void Unregister(AssetID model);
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

protected:
    void OnInit();
    void OnShutdown();

private:
    Fleur::Graphics::SFLShaderInfo shaderInfo(Fleur::Graphics::Shader* shader);

    Fleur::Graphics::IRenderer* m_Backend{nullptr};
    bool m_Vsync{true};
};
}  // namespace Lux
