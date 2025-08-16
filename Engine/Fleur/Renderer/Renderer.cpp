#include "Renderer.h"

#include <span>

namespace Fleur::Graphics
{
ShaderObject* shader_object;

uint32_t Renderer::MAX_TEXTURES_COUNT = 0;

Renderer::Renderer(GraphicsAPI api, std::unique_ptr<Fleur::IRendererToolchain> tool)
    : show_wireframe(false)
    , _camera(nullptr)
    , current_shader_obj(nullptr)
    , is_vsync(true)
    , renderer(api)
    , toolchain(std::move(tool))
{
}

std::shared_ptr<Fleur::Graphics::Texture> Renderer::load_texture(std::string_view path)
{
    if (path.empty())
        return GetLoadedTexture("fallback");

    // Try to find first across loaded textures:
    std::string name = std::filesystem::path(path.data()).stem().string();
    auto it = textures.find(name);
    if (it != textures.end())
        return it->second;

    // If not, load Image and then create new texture:
    std::shared_ptr<Fleur::Graphics::Image2D> image{nullptr};
    auto assets_manager = ServiceLocator::instance().GetService<AssetsManager>();
    auto existing_img = assets_manager->Get<Fleur::Graphics::Image2D>(name);
    if (!existing_img.expired())
        image = existing_img.lock();
    else
    {
        image = assets_manager->Load<Fleur::Graphics::Image2D>(path)->Resource();
        if (!image.get())
        {
            return GetLoadedTexture("fallback");
        }
    }

    auto texture = toolchain->LoadTexture(image, _device.get());
    return textures.emplace(image->Name(), texture).first->second;
}

std::shared_ptr<Fleur::Graphics::Texture> Renderer::load_texture(std::string_view name, Color color, int width, int height)
{
    if (name.empty())
        return GetLoadedTexture("fallback");

    // Try to find first across loaded textures:
    auto it = textures.find(name.data());
    if (it != textures.end())
        return it->second;

    // Create image
    std::shared_ptr<Fleur::Graphics::Image2D> image{nullptr};
    auto assets_manager = ServiceLocator::instance().GetService<AssetsManager>();
    auto existing_img = assets_manager->Get<Fleur::Graphics::Image2D>(name);
    if (!existing_img.expired())
        image = existing_img.lock();
    else
        image = assets_manager->LoadImage2DFromColor(name, color, width, height)->Resource();

    auto texture = toolchain->LoadTexture(image, _device.get());
    return textures.emplace(image->Name(), texture).first->second;
}

std::shared_ptr<Fleur::Graphics::Texture> Renderer::load_texture(std::shared_ptr<Fleur::Graphics::Image2D> img)
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

    auto& application = Fleur::Application::instance();

    _swapchain = _device->CreateSwapchain(_device->CreateSurface(application.GetWindow().GetNativeHandle()));

    _commandPool = _device->CreateCommandPool(*_commandQueue);

    auto static_geo_vs = _device->CreateShader("static_geo", Shader::ShaderType::Vertex);
    std::shared_ptr<ShaderObject> static_geometry_shader(
        ShaderObject::CreateShaderObject("static_geometry_shader", static_geo_vs, _device->CreateShader("static_geo", Shader::ShaderType::Pixel)));
    // Static geometry
    DepthStencilDescriptor desc{true, DepthTestOperation::LESS};
    static_geometry_cmd = _device->CreateCommandBuffer(desc);
    static_geometry_cmd->BindShaderObject(static_geometry_shader);

    VertexLayout layout{};
    layout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::DataType::FLOAT, true));
    layout.AddAttribute(VertexLayout::VertexAttribute(1, 2, VertexLayout::DataType::FLOAT, true));
    layout.AddAttribute(VertexLayout::VertexAttribute(2, 3, VertexLayout::DataType::FLOAT, true));
    static_geometry_cmd->BindVertexBuffer(_device->CreateBuffer(Fleur::Graphics::Buffer::BufferType::Vertex, STATIC_GEOMETRY, 100 * 1024 * 1024), layout);
    static_geometry_cmd->BindIndexBuffer(_device->CreateBuffer(Fleur::Graphics::Buffer::BufferType::Index, STATIC_GEOMETRY, 100 * 1024 * 1024));

    // Skybox
    std::shared_ptr<ShaderObject> skybox_shader(ShaderObject::CreateShaderObject("skybox_shader", _device->CreateShader("skybox", Shader::ShaderType::Vertex),
                                                                                 _device->CreateShader("skybox", Shader::ShaderType::Pixel)));
    DepthStencilDescriptor desc2{false, DepthTestOperation::LESS_OR_EQUAL};
    skybox_cmd = _device->CreateCommandBuffer(desc2);
    skybox_cmd->BindShaderObject(skybox_shader);

    VertexLayout skybox_layout{};
    skybox_layout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::DataType::FLOAT, true));
    skybox_cmd->BindVertexBuffer(_device->CreateBuffer(Fleur::Graphics::Buffer::BufferType::Vertex, STATIC_GEOMETRY, 108 * sizeof(float)), skybox_layout);

    // gizmo
    std::shared_ptr<ShaderObject> gizmo_shader(ShaderObject::CreateShaderObject("gizmo_shader", _device->CreateShader("static_geo", Shader::ShaderType::Vertex),
                                                                                _device->CreateShader("gizmo", Shader::ShaderType::Pixel)));
    DepthStencilDescriptor desc3{false, DepthTestOperation::ALWAYS};
    gizmo_cmd = _device->CreateCommandBuffer(desc3);
    gizmo_cmd->BindShaderObject(gizmo_shader);

    VertexLayout gizmo_layout{};
    gizmo_layout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::DataType::FLOAT, true));
    gizmo_layout.AddAttribute(VertexLayout::VertexAttribute(1, 2, VertexLayout::DataType::FLOAT, true));
    gizmo_layout.AddAttribute(VertexLayout::VertexAttribute(2, 3, VertexLayout::DataType::FLOAT, true));
    gizmo_cmd->BindVertexBuffer(_device->CreateBuffer(Fleur::Graphics::Buffer::BufferType::Vertex, STATIC_GEOMETRY, 500 * 1024), gizmo_layout);
    gizmo_cmd->BindIndexBuffer(_device->CreateBuffer(Fleur::Graphics::Buffer::BufferType::Index, STATIC_GEOMETRY, 500 * 1024));

    gizmo_fbo = _device->CreateFramebuffer("gizmo_framebuffer", application.GetWindow().GetWidth(), application.GetWindow().GetHeight(),
                                           (uint32_t)FramebufferSettings::COLOR | (uint32_t)FramebufferSettings::DEPTH_STENCIL);


    std::shared_ptr<ShaderObject> copy_fbo_shader(
        ShaderObject::CreateShaderObject("copy_fbo_as_quad_shader", static_geo_vs, _device->CreateShader("CopyFBOAsQuad", Shader::ShaderType::Pixel)));

    DepthStencilDescriptor desc4{true, DepthTestOperation::LESS};
    copy_fbo_cmd = _device->CreateCommandBuffer(desc4);
    copy_fbo_cmd->BindShaderObject(copy_fbo_shader);

    VertexLayout copy_fbo_layout{};
    copy_fbo_layout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::DataType::FLOAT, true));
    copy_fbo_layout.AddAttribute(VertexLayout::VertexAttribute(1, 2, VertexLayout::DataType::FLOAT, true));
    copy_fbo_cmd->BindVertexBuffer(_device->CreateBuffer(Fleur::Graphics::Buffer::BufferType::Vertex, STATIC_GEOMETRY, 500 * 1024), copy_fbo_layout);
    copy_fbo_cmd->BindIndexBuffer(_device->CreateBuffer(Fleur::Graphics::Buffer::BufferType::Index, STATIC_GEOMETRY, 500 * 1024));

    // clang-format off
    static float quadVertices[] = {
    -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
     1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,   0.0f, 1.0f,
     -1.0f, 1.0f, 0.0f,   1.0f, 1.0f,
     1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
     1.0f,  -1.0f, 0.0f,   1.0f, 1.0f
};
    // clang-format on
    copy_fbo_cmd->UpdateBufferSubData<float>(Buffer::Vertex, std::span(quadVertices, 30));
}

void Renderer::OnShutdown()
{
    _device->Release();
    _swapchain->Release();
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
            it->second.model_matrix = model_pos;
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
    case GIZMO:
    {
        auto it = gizmo_models.find(model->GetName().data());
        if (it != gizmo_models.end())
        {
            it->second.model_matrix = model_pos;
            return;
        }

        DrawInfo draw{model, model_pos};

        draw.vertex_global_offset_bytes =
            gizmo_cmd->UpdateBufferSubData<VertexData>(Buffer::Vertex, std::span(model->GetVerticesData(), model->GetVertexCount()));

        draw.index_global_offset_bytes = gizmo_cmd->UpdateBufferSubData<uint32_t>(Buffer::Index, std::span(model->GetIndicesData(), model->GetIndicesCount()));

        gizmo_models.emplace(model->GetName().data(), draw);
        gizmo_models_vector.emplace_back(draw);
        break;
    }
    case DYNAMIC_DRAW:
        break;
    }
}

void Renderer::Clear()
{
    _swapchain->ClearBackbuffer();
    gizmo_fbo->Clear();
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
    static bool is_skybox_created = false;

    auto assets_manager = ServiceLocator::instance().GetService<AssetsManager>();
    static Fleur::Graphics::CubemapInitData skybox_images;

    {
        auto cubemap = assets_manager->Get<CubemapImage>("skybox_cross_layout_cubemap");
        if (!cubemap.expired() && !is_skybox_created)
        {
            auto cube_map_texture = _device->CreateCubemap(cubemap.lock().get());

            float skyboxVertices[] = {
                -1.0f, 1.0f,  -1.0f,  // 0
                -1.0f, -1.0f, -1.0f,  // 1
                1.0f,  -1.0f, -1.0f,  // 2
                1.0f,  -1.0f, -1.0f,  // 3
                1.0f,  1.0f,  -1.0f,  // 4
                -1.0f, 1.0f,  -1.0f,  // 5
                -1.0f, -1.0f, 1.0f,   // 6
                -1.0f, -1.0f, -1.0f,  // 7
                -1.0f, 1.0f,  -1.0f,  // 8
                -1.0f, 1.0f,  -1.0f,  // 9
                -1.0f, 1.0f,  1.0f,   // 10
                -1.0f, -1.0f, 1.0f,   // 11
                1.0f,  -1.0f, -1.0f,  // 12
                1.0f,  -1.0f, 1.0f,   // 13
                1.0f,  1.0f,  1.0f,   // 14
                1.0f,  1.0f,  1.0f,   // 15
                1.0f,  1.0f,  -1.0f,  // 16
                1.0f,  -1.0f, -1.0f,  // 17
                -1.0f, -1.0f, 1.0f,   // 18
                -1.0f, 1.0f,  1.0f,   // 19
                1.0f,  1.0f,  1.0f,   // 20
                1.0f,  1.0f,  1.0f,   // 21
                1.0f,  -1.0f, 1.0f,   // 22
                -1.0f, -1.0f, 1.0f,   // 23
                -1.0f, 1.0f,  -1.0f,  // 24
                1.0f,  1.0f,  -1.0f,  // 25
                1.0f,  1.0f,  1.0f,   // 26
                1.0f,  1.0f,  1.0f,   // 27
                -1.0f, 1.0f,  1.0f,   // 28
                -1.0f, 1.0f,  -1.0f,  // 29
                -1.0f, -1.0f, -1.0f,  // 30
                -1.0f, -1.0f, 1.0f,   // 31
                1.0f,  -1.0f, -1.0f,  // 32
                1.0f,  -1.0f, -1.0f,  // 33
                -1.0f, -1.0f, 1.0f,   // 34
                1.0f,  -1.0f, 1.0f    // 35
            };

            _skybox.reset(new Skybox(cube_map_texture, std::span{skyboxVertices}));
            skybox_cmd->UpdateBufferSubData<float>(Buffer::BufferType::Vertex, std::span(_skybox->Data(), _skybox->GetVertexCount()));
            is_skybox_created = true;
        }
    }

    // Skybox pass
    skybox_pass();

    // Main Pass
    static_geometry_pass();

    // gizmo
    gizmo_cmd->PushDebugGroup(0, "[STAGE] -> Gizmo");
    gizmo_cmd->BindRenderTarget(*gizmo_fbo.get(), FramebufferRWOperation::WRITE_ONLY);
    gizmo_cmd->BeginRecording();

    gizmo_cmd->ShaderObject()->Use();

    for (const auto& draw_info : gizmo_models_vector)
    {
        gizmo_cmd->PushDebugGroup(0, draw_info.model->GetName().data());

        glm::mat4 proj = glm::mat4(1.0f);

        glm::mat4 view = glm::mat4(1.f);

        glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(-0.9f, -0.9f, 0.1f)), glm::vec3(0.05f, 0.05f, 0.05f));

        model = glm::rotate(model, glm::radians(_camera->Yaw()), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(_camera->Pitch()), glm::vec3(1.0f, 0.0f, 0.0f));

        gizmo_cmd->ShaderObject()->Set("projection", proj);
        gizmo_cmd->ShaderObject()->Set("model", model);
        gizmo_cmd->ShaderObject()->Set("view", view);

        const auto* meshes = draw_info.model->GetMeshesPtr();

        uint32_t index_inner_offset_bytes = 0;
        for (const auto& mesh : *meshes)
        {
            gizmo_cmd->PushDebugGroup(0, mesh.Name().data());
            for (uint32_t i = 0; i < mesh.PrimitivesCount(); i++)
            {
                gizmo_cmd->PushDebugGroup(0, "Primitive");
                auto primitive = mesh.Primitives() + i;
                gizmo_cmd->ShaderObject()->BindMaterial(draw_info.model->GetMaterial(primitive->MaterialIdx()));
                gizmo_cmd->IndexedDraw(primitive->IndexCount(), draw_info.index_global_offset_bytes + index_inner_offset_bytes,
                                       draw_info.vertex_global_offset_bytes / sizeof(VertexData));

                index_inner_offset_bytes += primitive->IndexSize();
                gizmo_cmd->PopDebugGroup();
            }

            gizmo_cmd->PopDebugGroup();
        }
        gizmo_cmd->PopDebugGroup();
        gizmo_cmd->EndRecording();
        gizmo_cmd->Submit();
    }
    gizmo_cmd->PopDebugGroup();

    static_geometry_cmd->BeginRecording();

    //
    copy_fbo_cmd->PushDebugGroup(0, "[Copy FBO]");
    gizmo_cmd->BindRenderTarget(_swapchain->GetScreenTexture(), FramebufferRWOperation::READ_WRITE);

    ShaderComponentContext ctx{};
    ctx.albedo_text.second = gizmo_fbo->GetColorAttachment(0);

    copy_fbo_cmd->ShaderObject()->Use();
    copy_fbo_cmd->ShaderObject()->BindMaterial(Material::CreateMaterial(ctx));

    copy_fbo_cmd->ShaderObject()->Set("projection", glm::mat4(1.0f));
    copy_fbo_cmd->ShaderObject()->Set("model", glm::mat4(1.0f));
    copy_fbo_cmd->ShaderObject()->Set("view", glm::mat4(1.0f));

    copy_fbo_cmd->Draw(6);

    copy_fbo_cmd->PopDebugGroup();
}

void Renderer::OnPostUpdate(float dlTime)
{
    // TODO
}

void Renderer::OnFixedUpdate()
{
    // TODO
}

void Renderer::UpdateViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    gizmo_fbo->Bind();
    uint32_t flags = gizmo_fbo->Flags();
    gizmo_fbo.reset(_device->CreateFramebuffer("gizmo_fbo", width, height, flags).release());
}

VertexData::VertexData(glm::vec3 pos, glm::vec3 text_coord, glm::vec3 normal)
    : pos(pos)
    , textcoord(text_coord)
    , normal(normal)
{
}

void Renderer::skybox_pass() const
{
    if (!_skybox)
        return;

    skybox_cmd->PushDebugGroup(0, "[STAGE] -> Skybox stage");
    skybox_cmd->BeginRecording();
    skybox_cmd->BindRenderTarget(_swapchain->GetScreenTexture(), FramebufferRWOperation::READ_WRITE);

    skybox_cmd->ShaderObject()->Use();

    skybox_cmd->ShaderObject()->Set("view", _camera->GetView());
    skybox_cmd->ShaderObject()->Set("projection", _camera->GetProjection());

    skybox_cmd->ShaderObject()->BindMaterial(_skybox->GetMaterial());
    skybox_cmd->Draw(_skybox->GetVertexCount() / 3);

    skybox_cmd->PopDebugGroup();
}

void Renderer::static_geometry_pass() const
{
    static_geometry_cmd->PushDebugGroup(0, "[STAGE] -> Static geometry stage");
    static_geometry_cmd->BindRenderTarget(_swapchain->GetScreenTexture(), FramebufferRWOperation::READ_WRITE);
    static_geometry_cmd->BeginRecording();

    static_geometry_cmd->ShaderObject()->Use();

    for (const auto& draw_info : static_geometry_models_vector)
    {
        static_geometry_cmd->PushDebugGroup(0, draw_info.model->GetName().data());
        static_geometry_cmd->ShaderObject()->Set("model", draw_info.model_matrix);
        static_geometry_cmd->ShaderObject()->Set("view", _camera->GetView());
        static_geometry_cmd->ShaderObject()->Set("projection", _camera->GetProjection());

        const auto* meshes = draw_info.model->GetMeshesPtr();

        uint32_t index_inner_offset_bytes = 0;
        for (const auto& mesh : *meshes)
        {
            for (uint32_t i = 0; i < mesh.PrimitivesCount(); i++)
            {
                const auto primitive = mesh.Primitives() + i;
                static_geometry_cmd->PushDebugGroup(0, mesh.Name().data());
                static_geometry_cmd->ShaderObject()->BindMaterial(draw_info.model->GetMaterial(primitive->MaterialIdx()));
                static_geometry_cmd->IndexedDraw(primitive->IndexCount(), draw_info.index_global_offset_bytes + index_inner_offset_bytes,
                                                 draw_info.vertex_global_offset_bytes / sizeof(VertexData));

                index_inner_offset_bytes += primitive->IndexSize();
                static_geometry_cmd->PopDebugGroup();
            }
        }
        static_geometry_cmd->PopDebugGroup();
        static_geometry_cmd->EndRecording();
        static_geometry_cmd->Submit();
    }
    static_geometry_cmd->PopDebugGroup();
}

}  // namespace Fleur::Graphics
