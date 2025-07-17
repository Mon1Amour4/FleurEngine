#include "Renderer.h"

#include <span>

namespace Fuego::Graphics
{
ShaderObject* shader_object;

uint32_t Renderer::MAX_TEXTURES_COUNT = 0;

Renderer::Renderer(GraphicsAPI api, std::unique_ptr<Fuego::IRendererToolchain> tool)
    : show_wireframe(false)
    , _camera(nullptr)
    , current_shader_obj(nullptr)
    , is_vsync(true)
    , renderer(api)
    , toolchain(std::move(tool))
{
}

std::shared_ptr<Fuego::Graphics::Texture> Renderer::load_texture(std::string_view path)
{
    if (path.empty())
        return GetLoadedTexture("fallback");

    // Try to find first across loaded textures:
    std::string name = std::filesystem::path(path.data()).stem().string();
    auto it = textures.find(name);
    if (it != textures.end())
        return it->second;

    // If not, load Image and then create new texture:
    std::shared_ptr<Fuego::Graphics::Image2D> image{nullptr};
    auto assets_manager = ServiceLocator::instance().GetService<AssetsManager>();
    auto existing_img = assets_manager->Get<Fuego::Graphics::Image2D>(name);
    if (!existing_img.expired())
        image = existing_img.lock();
    else
    {
        image = assets_manager->Load<Fuego::Graphics::Image2D>(path)->Resource();
        if (!image.get())
        {
            return GetLoadedTexture("fallback");
        }
    }

    auto texture = toolchain->LoadTexture(image, _device.get());
    return textures.emplace(image->Name(), texture).first->second;
}

std::shared_ptr<Fuego::Graphics::Texture> Renderer::load_texture(std::string_view name, Color color, int width, int height)
{
    if (name.empty())
        return GetLoadedTexture("fallback");

    // Try to find first across loaded textures:
    auto it = textures.find(name.data());
    if (it != textures.end())
        return it->second;

    // Create image
    std::shared_ptr<Fuego::Graphics::Image2D> image{nullptr};
    auto assets_manager = ServiceLocator::instance().GetService<AssetsManager>();
    auto existing_img = assets_manager->Get<Fuego::Graphics::Image2D>(name);
    if (!existing_img.expired())
        image = existing_img.lock();
    else
        image = assets_manager->LoadImage2DFromColor(name, color, width, height)->Resource();

    auto texture = toolchain->LoadTexture(image, _device.get());
    return textures.emplace(image->Name(), texture).first->second;
}

std::shared_ptr<Fuego::Graphics::Texture> Renderer::load_texture(std::shared_ptr<Fuego::Graphics::Image2D> img)
{
    if (!img)
        return GetLoadedTexture("fallback");

    std::string name = std::filesystem::path(img->Name()).stem().string();
    auto it = textures.find(name);
    if (it != textures.end())
        return it->second;

    auto texture = toolchain->LoadTexture(img, _device.get());
    return textures.emplace(img->Name(), texture).first->second;
}

void Renderer::OnInit()
{
    _camera.reset(new Camera());
    _camera->Activate();

    _device = Device::CreateDevice();
    _device->SetVSync(is_vsync);

    _commandQueue = _device->CreateCommandQueue();

    // Temporary: we're creating surface for main Application window
    _surface = _device->CreateSurface(Fuego::Application::instance().GetWindow().GetNativeHandle());
    _swapchain = _device->CreateSwapchain(*_surface);
    _commandPool = _device->CreateCommandPool(*_commandQueue);

    std::shared_ptr<ShaderObject> static_geometry_shader(ShaderObject::CreateShaderObject(_device->CreateShader("vs_shader", Shader::ShaderType::Vertex),
                                                                                          _device->CreateShader("ps_triangle", Shader::ShaderType::Pixel)));
    // Static geometry
    static_geometry_cmd = _device->CreateCommandBuffer();
    static_geometry_cmd->BindShaderObject(static_geometry_shader);

    VertexLayout layout{};
    layout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::DataType::FLOAT, true));
    layout.AddAttribute(VertexLayout::VertexAttribute(1, 2, VertexLayout::DataType::FLOAT, true));
    layout.AddAttribute(VertexLayout::VertexAttribute(2, 3, VertexLayout::DataType::FLOAT, true));
    static_geometry_cmd->BindVertexBuffer(_device->CreateBuffer(Fuego::Graphics::Buffer::BufferType::Vertex, STATIC_GEOMETRY, 100 * 1024 * 1024), layout);
    static_geometry_cmd->BindIndexBuffer(_device->CreateBuffer(Fuego::Graphics::Buffer::BufferType::Index, STATIC_GEOMETRY, 100 * 1024 * 1024));

    // Skybox
    std::shared_ptr<ShaderObject> skybox_shader(ShaderObject::CreateShaderObject(_device->CreateShader("skybox.vs", Shader::ShaderType::Vertex),
                                                                                 _device->CreateShader("skybox.ps", Shader::ShaderType::Pixel)));
    skybox_cmd = _device->CreateCommandBuffer();
    skybox_cmd->BindShaderObject(skybox_shader);

    VertexLayout skybox_layout{};
    skybox_layout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::DataType::FLOAT, true));
    skybox_cmd->BindVertexBuffer(_device->CreateBuffer(Fuego::Graphics::Buffer::BufferType::Vertex, STATIC_GEOMETRY, 108 * sizeof(float)), skybox_layout);
}

void Renderer::OnShutdown()
{
    _device->Release();
    _device.release();

    _surface->Release();
    _surface.release();
}

std::shared_ptr<Texture> Renderer::GetLoadedTexture(std::string_view path) const
{
    if (path.empty())
        return textures.find("fallback")->second;

    std::string name = std::filesystem::path(path.data()).stem().string();

    auto it = textures.find(name.data());
    if (it != textures.end())
        return it->second;
    else
        return textures.find("fallback")->second;
}

void Renderer::DrawModel(RenderStage stage, const Model* model, glm::mat4 model_pos)
{
    switch (stage)
    {
    case STATIC_GEOMETRY:
    {
        auto it = static_geometry_models.find(model->GetName().data());
        if (it != static_geometry_models.end())
        {
            it->second.pos = model_pos;
            return;
        }

        DrawInfo draw{model, model_pos};

        draw.vertex_global_offset_bytes =
            static_geometry_cmd->UpdateBufferSubData<VertexData>(Buffer::Vertex, std::span(model->GetVerticesData(), model->GetVertexCount()));

        draw.index_global_offset_bytes =
            static_geometry_cmd->UpdateBufferSubData<uint32_t>(Buffer::Index, std::span(model->GetIndicesData(), model->GetIndicesCount()));

        static_geometry_models.emplace(model->GetName().data(), draw);
        static_geometry_models_vector.emplace_back(draw);
        break;
    }
    case DYNAMIC_DRAW:
        break;
    }
}

void Renderer::Clear()
{
    // CommandBuffer& cmd = _commandPool->GetCommandBuffer();
    // cmd.Clear();
    _surface->Clear();
}

void Renderer::Present()
{
    _swapchain->Present();
}

void Renderer::ShowWireFrame()
{
    if (show_wireframe)
    {
        _swapchain->ShowWireFrame(true);
    }
    else
    {
        _swapchain->ShowWireFrame(false);
    }
}

void Renderer::ToggleWireFrame()
{
    show_wireframe = !show_wireframe;
}

void Renderer::ValidateWindow()
{
    _swapchain->ValidateWindow();
}

void Renderer::SetVSync(bool active)
{
    is_vsync = active;
    _device->SetVSync(is_vsync);
}

bool Renderer::IsVSync()
{
    return is_vsync;
}

void Renderer::OnUpdate(float dlTime)
{
    toolchain->Update();

    auto assets_manager = ServiceLocator::instance().GetService<AssetsManager>();

    auto cubemap = assets_manager->Get<CubemapImage>("skybox");
    static int b = 1;
    if (!cubemap.expired() && b == 1)
    {
        auto cube_map_texture = _device->CreateCubemap(cubemap.lock().get());

        // clang-format off
        float skyboxVertices[] = {  -1.0f,  1.0f, -1.0f, // 0
                                    -1.0f, -1.0f, -1.0f, // 1
                                     1.0f, -1.0f, -1.0f, // 2
                                     1.0f, -1.0f, -1.0f, // 3
                                     1.0f,  1.0f, -1.0f, // 4
                                    -1.0f,  1.0f, -1.0f, // 5
                                    -1.0f, -1.0f,  1.0f, // 6
                                    -1.0f, -1.0f, -1.0f, // 7
                                    -1.0f,  1.0f, -1.0f, // 8
                                    -1.0f,  1.0f, -1.0f, // 9
                                    -1.0f,  1.0f,  1.0f, // 10
                                    -1.0f, -1.0f,  1.0f, // 11
                                     1.0f, -1.0f, -1.0f, // 12
                                     1.0f, -1.0f,  1.0f, // 13
                                     1.0f,  1.0f,  1.0f, // 14
                                     1.0f,  1.0f,  1.0f, // 15
                                     1.0f,  1.0f, -1.0f, // 16
                                     1.0f, -1.0f, -1.0f, // 17
                                    -1.0f, -1.0f,  1.0f, // 18 
                                    -1.0f,  1.0f,  1.0f, // 19
                                     1.0f,  1.0f,  1.0f, // 20
                                     1.0f,  1.0f,  1.0f, // 21
                                     1.0f, -1.0f,  1.0f, // 22
                                    -1.0f, -1.0f,  1.0f, // 23
                                    -1.0f,  1.0f, -1.0f, // 24
                                     1.0f,  1.0f, -1.0f, // 25
                                     1.0f,  1.0f,  1.0f, // 26
                                     1.0f,  1.0f,  1.0f, // 27
                                    -1.0f,  1.0f,  1.0f, // 28
                                    -1.0f,  1.0f, -1.0f, // 29
                                    -1.0f, -1.0f, -1.0f, // 30
                                    -1.0f, -1.0f,  1.0f, // 31
                                     1.0f, -1.0f, -1.0f, // 32
                                     1.0f, -1.0f, -1.0f, // 33
                                    -1.0f, -1.0f,  1.0f, // 34
                                     1.0f, -1.0f,  1.0f  // 35
        };
        // clang-format on

        _skybox.reset(new Skybox(cube_map_texture, std::span{skyboxVertices}));
        skybox_cmd->UpdateBufferSubData<float>(Buffer::BufferType::Vertex, std::span(_skybox->Data(), _skybox->GetVertexCount()));
        b++;
    }

    // Skybox pass
    skybox_pass();

    // Main Pass
    static_geometry_cmd->PushDebugGroup(0, "[PASS] -> Main Pass");
    static_geometry_cmd->PushDebugGroup(0, "[STAGE] -> Static geometry stage");
    static_geometry_cmd->BeginRecording();
    static_geometry_cmd->BindRenderTarget(_swapchain->GetScreenTexture());
    static_geometry_cmd->SetDepthWriting(true);

    static_geometry_cmd->ShaderObject()->Use();

    for (const auto& draw_info : static_geometry_models_vector)
    {
        static_geometry_cmd->PushDebugGroup(0, draw_info.model->GetName().data());
        static_geometry_cmd->ShaderObject()->Set("model", draw_info.pos);
        static_geometry_cmd->ShaderObject()->Set("view", _camera->GetView());
        static_geometry_cmd->ShaderObject()->Set("projection", _camera->GetProjection());

        const auto* meshes = draw_info.model->GetMeshesPtr();

        uint32_t index_inner_offset_bytes = 0;
        for (const auto& mesh : *meshes)
        {
            static_geometry_cmd->PushDebugGroup(0, mesh->Name().data());
            static_geometry_cmd->ShaderObject()->BindMaterial(mesh->GetMaterial());
            static_geometry_cmd->IndexedDraw(mesh->GetIndicesCount(), draw_info.index_global_offset_bytes + index_inner_offset_bytes,
                                             draw_info.vertex_global_offset_bytes / sizeof(VertexData));

            index_inner_offset_bytes += mesh->GetIndicesCount() * sizeof(uint32_t);
            static_geometry_cmd->PopDebugGroup();
        }
        static_geometry_cmd->PopDebugGroup();
        static_geometry_cmd->EndRecording();
        static_geometry_cmd->Submit();
    }
    static_geometry_cmd->PopDebugGroup();
    static_geometry_cmd->PopDebugGroup();
}

void Renderer::OnPostUpdate(float dlTime)
{
    // TODO
}

void Renderer::OnFixedUpdate()
{
    // TODO
}

void Renderer::ChangeViewport(float x, float y, float w, float h)
{
    viewport.x = x;
    viewport.y = y;
    viewport.width = w;
    viewport.height = h;
    UpdateViewport();
}

void Renderer::UpdateViewport()
{
    _surface.release();
    _surface = _device->CreateSurface(Fuego::Application::instance().GetWindow().GetNativeHandle());
    _swapchain.release();
    _swapchain = _device->CreateSwapchain(*_surface);
}

VertexData::VertexData(glm::vec3 pos, glm::vec3 text_coord, glm::vec3 normal)
    : pos(pos)
    , textcoord(text_coord)
    , normal(normal)
{
}

void Renderer::skybox_pass() const
{
    skybox_cmd->SetDepthWriting(false);
    skybox_cmd->PushDebugGroup(0, "[PASS] -> Skybox Pass");
    skybox_cmd->PushDebugGroup(0, "[STAGE] -> Skybox stage");
    skybox_cmd->BeginRecording();
    skybox_cmd->BindRenderTarget(_swapchain->GetScreenTexture());

    skybox_cmd->ShaderObject()->Use();

    skybox_cmd->ShaderObject()->Set("view", _camera->GetView());
    skybox_cmd->ShaderObject()->Set("projection", _camera->GetProjection());

    skybox_cmd->ShaderObject()->BindMaterial(_skybox->GetMaterial());
    skybox_cmd->Draw(_skybox->GetVertexCount() / 3);

    skybox_cmd->PopDebugGroup();
    skybox_cmd->PopDebugGroup();
}

}  // namespace Fuego::Graphics
