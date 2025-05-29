#pragma once

#include "Application.h"
#include "ApplicationPipeline.hpp"
#include "Buffer.h"
#include "Camera.h"
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "CommandQueue.h"
#include "Device.h"
#include "Graphics.hpp"
#include "Image2D.h"
#include "Material.h"
#include "Model.h"
#include "Services/ServiceInterfaces.hpp"
#include "Shader.h"
#include "ShaderObject.h"
#include "Surface.h"
#include "Swapchain.h"
#include "Texture.h"
#include "VertexLayout.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"

namespace Fuego::Graphics
{

class FUEGO_API Renderer : public Service<Renderer>, public IUpdatable
{
public:
    friend struct Service<Renderer>;

    Renderer(GraphicsAPI api, Fuego::Pipeline::Toolchain::renderer& toolchain);
    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&&) noexcept = default;
    Renderer& operator=(Renderer&&) noexcept = default;

    std::shared_ptr<Texture> CreateTexture(std::shared_ptr<Image2D> img);
    std::shared_ptr<Texture> GetLoadedTexture(std::string_view path) const;

    // IRenderer;
    void DrawModel(const Model* model, glm::mat4 model_pos);
    void ChangeViewport(float x, float y, float w, float h);

    // IUpdatable
    void OnUpdate(float dlTime);
    void OnPostUpdate(float dlTime);
    void OnFixedUpdate();

    void Clear();
    void Present();

    void ShowWireFrame();
    void ToggleWireFrame();
    void ValidateWindow();
    inline const Viewport& GetViewport() const
    {
        return viewport;
    }

    void SetVSync(bool active);
    bool IsVSync();

    static uint32_t MAX_TEXTURES_COUNT;

    inline const ShaderObject* CurrentShaderObject() const
    {
        return current_shader_obj;
    }
    inline void SetShaderObject(ShaderObject* obj)
    {
        current_shader_obj = obj;
    }

    std::unique_ptr<ShaderObject> opaque_shader;

private:
    bool show_wireframe;

    void UpdateViewport();
    std::unique_ptr<Device> _device;
    std::unique_ptr<CommandQueue> _commandQueue;
    std::unique_ptr<CommandPool> _commandPool;
    std::unique_ptr<Swapchain> _swapchain;
    std::unique_ptr<Surface> _surface;
    std::unique_ptr<Camera> _camera;


    ShaderObject* current_shader_obj;

    Viewport viewport;

    bool is_vsync;

    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
    GraphicsAPI renderer;
    Fuego::Pipeline::Toolchain::renderer toolchain;

    std::mutex texture_queue_mx;
    // Service
protected:
    void OnInit();
    void OnShutdown();
};
}  // namespace Fuego::Graphics
