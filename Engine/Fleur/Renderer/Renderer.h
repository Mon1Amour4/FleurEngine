#pragma once

#include "Application.h"
#include "Buffer.h"
#include "Camera.h"
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "CommandQueue.h"
#include "Device.h"
#include "Framebuffer.h"
#include "Graphics.hpp"
#include "Image2D.h"
#include "Material.h"
#include "Model.h"
#include "Services/ServiceInterfaces.hpp"
#include "Shader.h"
#include "ShaderObject.h"
#include "Skybox.h"
#include "Surface.h"
#include "Swapchain.h"
#include "Texture.h"
#include "Toolchain.h"
#include "VertexLayout.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "tbb/concurrent_unordered_map.h"

#pragma region concepts
template <class Resource>
concept is_graphic_resource = requires(Resource t) {
    std::is_same_v<std::remove_cv_t<Resource>, Fleur::Graphics::Texture> || std::is_same_v<std::remove_cv_t<Resource>, Fleur::Graphics::Shader>;
};

#pragma endregion

namespace Fleur::Graphics
{


class FLEUR_API Renderer : public Service<Renderer>, public IUpdatable
{
public:
    friend struct Service<Renderer>;

    Renderer(GraphicsAPI api, std::unique_ptr<Fleur::IRendererToolchain> tool);
    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&&) noexcept = default;
    Renderer& operator=(Renderer&&) noexcept = default;

    std::shared_ptr<Texture> GetLoadedTexture(std::string_view path) const;

    // IRenderer;
    void DrawModel(RenderStage stage, const Model* model, glm::mat4 modelPos);

    // IUpdatable
    void OnUpdate(float dlTime);
    void OnPostUpdate(float dlTime);
    void OnFixedUpdate();

    void Clear();
    void Present();

    void ShowWireFrame();
    void ToggleWireFrame();
    void ValidateWindow();

    void SetVSync(bool active);
    bool IsVSync();

    static uint32_t MAX_TEXTURES_COUNT;

    template <is_graphic_resource Resource, typename... Args>
    std::shared_ptr<Resource> CreateGraphicsResource(Args&&... args)
    {
        constexpr uint32_t args_amount = sizeof...(Args);
        if constexpr (std::is_same_v<std::remove_cv_t<Resource>, Fleur::Graphics::Texture>)
        {
            if constexpr (args_amount == 1)
            {
                using FirstArg = std::tuple_element_t<0, std::tuple<Args...>>;
                if constexpr ((std::is_same_v<std::remove_cv_t<std::remove_reference_t<FirstArg>>, std::shared_ptr<Fleur::Graphics::Image2D>>) ||
                              (std::is_same_v<FirstArg, std::string_view>) || (std::is_same_v<FirstArg, std::string>) ||
                              (std::is_convertible_v<FirstArg, const char*>))
                {
                    return Load_Texture(std::forward<Args>(args)...);
                }
            }
            else if constexpr (args_amount == 4)
            {
                return Load_Texture(std::forward<Args>(args)...);
            }
        }

        else if constexpr (std::is_same_v<std::remove_cv_t<Resource>, Fleur::Graphics::Shader>)
            return m_Device->CreateShader(std::forward<Args>(args)...);
        return std::shared_ptr<Resource>{nullptr};
    }
    void UpdateViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

private:
    bool show_wireframe;

    std::unique_ptr<Device> m_Device;
    std::unique_ptr<CommandQueue> m_CommandQueue;
    std::unique_ptr<CommandPool> m_CommandPool;
    std::unique_ptr<Swapchain> m_Swapchain;
    std::unique_ptr<Camera> m_Camera;
    std::unique_ptr<Skybox> m_Skybox;

    std::unique_ptr<CommandBuffer> m_StaticGeometryCmd;
    std::unique_ptr<CommandBuffer> m_SkyboxCmd;
    std::unique_ptr<CommandBuffer> m_GizmoCmd;
    std::unique_ptr<CommandBuffer> m_CopyFBOCmd;

    // TODO: move from here
    std::unique_ptr<Framebuffer> m_GizmoFBO;

    ShaderObject* m_CurrentShaderObj;

    bool m_IsVsync;

    tbb::concurrent_unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
    GraphicsAPI m_Renderer;

    std::unique_ptr<Fleur::IRendererToolchain> m_Toolchain;

    std::shared_ptr<Fleur::Graphics::Texture> Load_Texture(std::string_view path);

    std::shared_ptr<Fleur::Graphics::Texture> Load_Texture(std::string_view name, Color color, int width, int height);

    std::shared_ptr<Fleur::Graphics::Texture> Load_Texture(std::shared_ptr<Fleur::Graphics::Image2D> img);

    struct DrawInfo
    {
        const Model* Model;
        glm::mat4 ModelMatrix;
        uint32_t IndexGlobalOffsetBytes;
        uint32_t VertexGlobalOffsetBytes;
    };

    std::unordered_map<std::string, DrawInfo> m_StaticGeometryModels;
    std::unordered_map<std::string, DrawInfo> m_GizmoModels;
    std::vector<DrawInfo> m_StaticGeometryModelsVector;
    std::vector<DrawInfo> m_GizmoModelsVector;

    // Service

    void SkyboxPass() const;
    void StaticGeometryPass() const;

protected:
    void OnInit();
    void OnShutdown();
};
}  // namespace Fleur::Graphics
