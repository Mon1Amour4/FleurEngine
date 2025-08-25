#include "Renderer.h"

#include <span>

namespace Fleur::Graphics
{
ShaderObject* shader_object;

uint32_t Renderer::MAX_TEXTURES_COUNT = 0;

Renderer::Renderer(GraphicsAPI api, std::unique_ptr<Fleur::IRendererToolchain> tool)
    : show_wireframe(false)
    , m_Camera(nullptr)
    , m_CurrentShaderObj(nullptr)
    , m_IsVsync(true)
    , m_Renderer(api)
    , m_Toolchain(std::move(tool))
{
}

std::shared_ptr<Fleur::Graphics::Texture> Renderer::Load_Texture(std::string_view path)
{
    if (path.empty())
        return GetLoadedTexture("fallback");

    // Try to find first across loaded textures:
    std::string name = std::filesystem::path(path.data()).stem().string();
    auto it = m_Textures.find(name);
    if (it != m_Textures.end())
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

    auto texture = m_Toolchain->LoadTexture(image, m_Device.get());
    return m_Textures.emplace(image->Name(), texture).first->second;
}

std::shared_ptr<Fleur::Graphics::Texture> Renderer::Load_Texture(std::string_view name, Color color, int width, int height)
{
    if (name.empty())
        return GetLoadedTexture("fallback");

    // Try to find first across loaded textures:
    auto it = m_Textures.find(name.data());
    if (it != m_Textures.end())
        return it->second;

    // Create image
    std::shared_ptr<Fleur::Graphics::Image2D> image{nullptr};
    auto assets_manager = ServiceLocator::instance().GetService<AssetsManager>();
    auto existing_img = assets_manager->Get<Fleur::Graphics::Image2D>(name);
    if (!existing_img.expired())
        image = existing_img.lock();
    else
        image = assets_manager->LoadImage2DFromColor(name, color, width, height)->Resource();

    auto texture = m_Toolchain->LoadTexture(image, m_Device.get());
    return m_Textures.emplace(image->Name(), texture).first->second;
}

std::shared_ptr<Fleur::Graphics::Texture> Renderer::Load_Texture(std::shared_ptr<Fleur::Graphics::Image2D> img)
{
    if (!img)
        return GetLoadedTexture("fallback");

    std::string name = std::filesystem::path(img->Name()).stem().string();
    auto it = m_Textures.find(name);
    if (it != m_Textures.end())
        return it->second;

    auto texture = m_Toolchain->LoadTexture(img, m_Device.get());
    return m_Textures.emplace(img->Name(), texture).first->second;
}

void Renderer::OnInit()
{
    m_Camera.reset(new Camera());
    m_Camera->Activate();

    m_Device = Device::CreateDevice();
    m_Device->SetVSync(m_IsVsync);

    m_CommandQueue = m_Device->CreateCommandQueue();

    auto& application = Fleur::Application::instance();

    m_Swapchain = m_Device->CreateSwapchain(m_Device->CreateSurface(application.GetWindow().GetNativeHandle()));

    m_CommandPool = m_Device->CreateCommandPool(*m_CommandQueue);

    auto static_geo_vs = m_Device->CreateShader("static_geo", Shader::ShaderType::Vertex);
    std::shared_ptr<ShaderObject> static_geometry_shader(
        ShaderObject::CreateShaderObject("static_geometry_shader", static_geo_vs, m_Device->CreateShader("static_geo", Shader::ShaderType::Pixel)));
    // Static geometry
    DepthStencilDescriptor desc{true, EDepthTestOperation::LESS};
    m_StaticGeometryCmd = m_Device->CreateCommandBuffer(desc);
    m_StaticGeometryCmd->BindShaderObject(static_geometry_shader);

    VertexLayout layout{};
    layout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::EDataType::FLOAT, true));
    layout.AddAttribute(VertexLayout::VertexAttribute(1, 2, VertexLayout::EDataType::FLOAT, true));
    layout.AddAttribute(VertexLayout::VertexAttribute(2, 3, VertexLayout::EDataType::FLOAT, true));
    m_StaticGeometryCmd->BindVertexBuffer(m_Device->CreateBuffer(Fleur::Graphics::Buffer::EBufferType::Vertex, STATIC_GEOMETRY, 100 * 1024 * 1024), layout);
    m_StaticGeometryCmd->BindIndexBuffer(m_Device->CreateBuffer(Fleur::Graphics::Buffer::EBufferType::Index, STATIC_GEOMETRY, 100 * 1024 * 1024));

    // Skybox
    std::shared_ptr<ShaderObject> skybox_shader(ShaderObject::CreateShaderObject("skybox_shader", m_Device->CreateShader("skybox", Shader::ShaderType::Vertex),
                                                                                 m_Device->CreateShader("skybox", Shader::ShaderType::Pixel)));
    DepthStencilDescriptor desc2{false, EDepthTestOperation::LESS_OR_EQUAL};
    m_SkyboxCmd = m_Device->CreateCommandBuffer(desc2);
    m_SkyboxCmd->BindShaderObject(skybox_shader);

    VertexLayout skybox_layout{};
    skybox_layout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::EDataType::FLOAT, true));
    m_SkyboxCmd->BindVertexBuffer(m_Device->CreateBuffer(Fleur::Graphics::Buffer::EBufferType::Vertex, STATIC_GEOMETRY, 108 * sizeof(float)), skybox_layout);

    // gizmo
    std::shared_ptr<ShaderObject> gizmo_shader(ShaderObject::CreateShaderObject("gizmo_shader", m_Device->CreateShader("static_geo", Shader::ShaderType::Vertex),
                                                                                m_Device->CreateShader("gizmo", Shader::ShaderType::Pixel)));
    DepthStencilDescriptor desc3{false, EDepthTestOperation::ALWAYS};
    m_GizmoCmd = m_Device->CreateCommandBuffer(desc3);
    m_GizmoCmd->BindShaderObject(gizmo_shader);

    VertexLayout gizmo_layout{};
    gizmo_layout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::EDataType::FLOAT, true));
    gizmo_layout.AddAttribute(VertexLayout::VertexAttribute(1, 2, VertexLayout::EDataType::FLOAT, true));
    gizmo_layout.AddAttribute(VertexLayout::VertexAttribute(2, 3, VertexLayout::EDataType::FLOAT, true));
    m_GizmoCmd->BindVertexBuffer(m_Device->CreateBuffer(Fleur::Graphics::Buffer::EBufferType::Vertex, STATIC_GEOMETRY, 500 * 1024), gizmo_layout);
    m_GizmoCmd->BindIndexBuffer(m_Device->CreateBuffer(Fleur::Graphics::Buffer::EBufferType::Index, STATIC_GEOMETRY, 500 * 1024));

    m_GizmoFBO = m_Device->CreateFramebuffer("gizmo_framebuffer", application.GetWindow().GetWidth(), application.GetWindow().GetHeight(),
                                           (uint32_t)FramebufferSettings::COLOR | (uint32_t)FramebufferSettings::DEPTH_STENCIL);


    std::shared_ptr<ShaderObject> copy_fbo_shader(
        ShaderObject::CreateShaderObject("copy_fbo_as_quad_shader", static_geo_vs, m_Device->CreateShader("CopyFBOAsQuad", Shader::ShaderType::Pixel)));

    DepthStencilDescriptor desc4{true, EDepthTestOperation::LESS};
    m_CopyFBOCmd = m_Device->CreateCommandBuffer(desc4);
    m_CopyFBOCmd->BindShaderObject(copy_fbo_shader);

    VertexLayout copy_fbo_layout{};
    copy_fbo_layout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::EDataType::FLOAT, true));
    copy_fbo_layout.AddAttribute(VertexLayout::VertexAttribute(1, 2, VertexLayout::EDataType::FLOAT, true));
    m_CopyFBOCmd->BindVertexBuffer(m_Device->CreateBuffer(Fleur::Graphics::Buffer::EBufferType::Vertex, STATIC_GEOMETRY, 500 * 1024), copy_fbo_layout);
    m_CopyFBOCmd->BindIndexBuffer(m_Device->CreateBuffer(Fleur::Graphics::Buffer::EBufferType::Index, STATIC_GEOMETRY, 500 * 1024));

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
    m_CopyFBOCmd->UpdateBufferSubData<float>(Buffer::Vertex, std::span(quadVertices, 30));
}

void Renderer::OnShutdown()
{
    m_Device->Release();
    m_Swapchain->Release();
}

std::shared_ptr<Texture> Renderer::GetLoadedTexture(std::string_view path) const
{
    if (path.empty())
        return m_Textures.find("fallback")->second;

    std::string name = std::filesystem::path(path.data()).stem().string();

    auto it = m_Textures.find(name.data());
    if (it != m_Textures.end())
        return it->second;
    else
        return m_Textures.find("fallback")->second;
}

void Renderer::DrawModel(RenderStage stage, const Model* model, glm::mat4 model_pos)
{
    switch (stage)
    {
    case STATIC_GEOMETRY:
    {
        auto it = m_StaticGeometryModels.find(model->GetName().data());
        if (it != m_StaticGeometryModels.end())
        {
            it->second.ModelMatrix = model_pos;
            return;
        }

        DrawInfo draw{model, model_pos};

        draw.VertexGlobalOffsetBytes =
            m_StaticGeometryCmd->UpdateBufferSubData<VertexData>(Buffer::Vertex, std::span(model->GetVerticesData(), model->GetVertexCount()));

        draw.IndexGlobalOffsetBytes =
            m_StaticGeometryCmd->UpdateBufferSubData<uint32_t>(Buffer::Index, std::span(model->GetIndicesData(), model->GetIndicesCount()));

        m_StaticGeometryModels.emplace(model->GetName().data(), draw);
        m_StaticGeometryModelsVector.emplace_back(draw);
        break;
    }
    case GIZMO:
    {
        auto it = m_GizmoModels.find(model->GetName().data());
        if (it != m_GizmoModels.end())
        {
            it->second.ModelMatrix = model_pos;
            return;
        }

        DrawInfo draw{model, model_pos};

        draw.VertexGlobalOffsetBytes =
            m_GizmoCmd->UpdateBufferSubData<VertexData>(Buffer::Vertex, std::span(model->GetVerticesData(), model->GetVertexCount()));

        draw.IndexGlobalOffsetBytes = m_GizmoCmd->UpdateBufferSubData<uint32_t>(Buffer::Index, std::span(model->GetIndicesData(), model->GetIndicesCount()));

        m_GizmoModels.emplace(model->GetName().data(), draw);
        m_GizmoModelsVector.emplace_back(draw);
        break;
    }
    case DYNAMIC_DRAW:
        break;
    }
}

void Renderer::Clear()
{
    m_Swapchain->ClearBackbuffer();
    m_GizmoFBO->Clear();
}

void Renderer::Present()
{
    m_Swapchain->Present();
}

void Renderer::ShowWireFrame()
{
    if (show_wireframe)
    {
        m_Swapchain->ShowWireFrame(true);
    }
    else
    {
        m_Swapchain->ShowWireFrame(false);
    }
}

void Renderer::ToggleWireFrame()
{
    show_wireframe = !show_wireframe;
}

void Renderer::ValidateWindow()
{
    m_Swapchain->ValidateWindow();
}

void Renderer::SetVSync(bool active)
{
    m_IsVsync = active;
    m_Device->SetVSync(m_IsVsync);
}

bool Renderer::IsVSync()
{
    return m_IsVsync;
}

void Renderer::OnUpdate(float dlTime)
{
    m_Toolchain->Update();
    static bool is_skybox_created = false;

    auto assets_manager = ServiceLocator::instance().GetService<AssetsManager>();
    static Fleur::Graphics::CubemapInitData skybox_images;

    {
        auto cubemap = assets_manager->Get<CubemapImage>("skybox_cross_layout_cubemap");
        if (!cubemap.expired() && !is_skybox_created)
        {
            auto cube_map_texture = m_Device->CreateCubemap(cubemap.lock().get());

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

            m_Skybox.reset(new Skybox(cube_map_texture, std::span{skyboxVertices}));
            m_SkyboxCmd->UpdateBufferSubData<float>(Buffer::EBufferType::Vertex, std::span(m_Skybox->Data(), m_Skybox->GetVertexCount()));
            is_skybox_created = true;
        }
    }

    // Skybox pass
    SkyboxPass();

    // Main Pass
    StaticGeometryPass();

    // gizmo
    m_GizmoCmd->PushDebugGroup(0, "[STAGE] -> Gizmo");
    m_GizmoCmd->BindRenderTarget(*m_GizmoFBO.get(), FramebufferRWOperation::WRITE_ONLY);
    m_GizmoCmd->BeginRecording();

    m_GizmoCmd->ShaderObject()->Use();

    for (const auto& draw_info : m_GizmoModelsVector)
    {
        m_GizmoCmd->PushDebugGroup(0, draw_info.Model->GetName().data());

        glm::mat4 proj = glm::mat4(1.0f);

        glm::mat4 view = glm::mat4(1.f);

        glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(-0.9f, -0.9f, 0.1f)), glm::vec3(0.05f, 0.05f, 0.05f));

        model = glm::rotate(model, glm::radians(m_Camera->Yaw()), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(m_Camera->Pitch()), glm::vec3(1.0f, 0.0f, 0.0f));

        m_GizmoCmd->ShaderObject()->Set("projection", proj);
        m_GizmoCmd->ShaderObject()->Set("model", model);
        m_GizmoCmd->ShaderObject()->Set("view", view);

        const auto* meshes = draw_info.Model->GetMeshesPtr();

        uint32_t index_inner_offset_bytes = 0;
        for (const auto& mesh : *meshes)
        {
            m_GizmoCmd->PushDebugGroup(0, mesh.Name().data());
            for (uint32_t i = 0; i < mesh.PrimitivesCount(); i++)
            {
                m_GizmoCmd->PushDebugGroup(0, "Primitive");
                auto primitive = mesh.Primitives() + i;
                m_GizmoCmd->ShaderObject()->BindMaterial(draw_info.Model->GetMaterial(primitive->MaterialIdx()));
                m_GizmoCmd->IndexedDraw(primitive->IndexCount(), draw_info.IndexGlobalOffsetBytes + index_inner_offset_bytes,
                                       draw_info.VertexGlobalOffsetBytes / sizeof(VertexData));

                index_inner_offset_bytes += primitive->IndexSize();
                m_GizmoCmd->PopDebugGroup();
            }

            m_GizmoCmd->PopDebugGroup();
        }
        m_GizmoCmd->PopDebugGroup();
        m_GizmoCmd->EndRecording();
        m_GizmoCmd->Submit();
    }
    m_GizmoCmd->PopDebugGroup();

    m_StaticGeometryCmd->BeginRecording();

    //
    m_CopyFBOCmd->PushDebugGroup(0, "[Copy FBO]");
    m_GizmoCmd->BindRenderTarget(m_Swapchain->GetScreenTexture(), FramebufferRWOperation::READ_WRITE);

    ShaderComponentContext ctx{};
    ctx.albedo_text.second = m_GizmoFBO->GetColorAttachment(0);

    m_CopyFBOCmd->ShaderObject()->Use();
    m_CopyFBOCmd->ShaderObject()->BindMaterial(Material::CreateMaterial(ctx));

    m_CopyFBOCmd->ShaderObject()->Set("projection", glm::mat4(1.0f));
    m_CopyFBOCmd->ShaderObject()->Set("model", glm::mat4(1.0f));
    m_CopyFBOCmd->ShaderObject()->Set("view", glm::mat4(1.0f));

    m_CopyFBOCmd->Draw(6);

    m_CopyFBOCmd->PopDebugGroup();
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
    m_GizmoFBO->Bind();
    uint32_t flags = m_GizmoFBO->Flags();
    m_GizmoFBO.reset(m_Device->CreateFramebuffer("gizmo_fbo", width, height, flags).release());
}

VertexData::VertexData(glm::vec3 pos, glm::vec3 text_coord, glm::vec3 normal)
    : pos(pos)
    , textcoord(text_coord)
    , normal(normal)
{
}

void Renderer::SkyboxPass() const
{
    if (!m_Skybox)
        return;

    m_SkyboxCmd->PushDebugGroup(0, "[STAGE] -> Skybox stage");
    m_SkyboxCmd->BeginRecording();
    m_SkyboxCmd->BindRenderTarget(m_Swapchain->GetScreenTexture(), FramebufferRWOperation::READ_WRITE);

    m_SkyboxCmd->ShaderObject()->Use();

    m_SkyboxCmd->ShaderObject()->Set("view", m_Camera->GetView());
    m_SkyboxCmd->ShaderObject()->Set("projection", m_Camera->GetProjection());

    m_SkyboxCmd->ShaderObject()->BindMaterial(m_Skybox->GetMaterial());
    m_SkyboxCmd->Draw(m_Skybox->GetVertexCount() / 3);

    m_SkyboxCmd->PopDebugGroup();
}

void Renderer::StaticGeometryPass() const
{
    m_StaticGeometryCmd->PushDebugGroup(0, "[STAGE] -> Static geometry stage");
    m_StaticGeometryCmd->BindRenderTarget(m_Swapchain->GetScreenTexture(), FramebufferRWOperation::READ_WRITE);
    m_StaticGeometryCmd->BeginRecording();

    m_StaticGeometryCmd->ShaderObject()->Use();

    for (const auto& draw_info : m_StaticGeometryModelsVector)
    {
        m_StaticGeometryCmd->PushDebugGroup(0, draw_info.Model->GetName().data());
        m_StaticGeometryCmd->ShaderObject()->Set("model", draw_info.ModelMatrix);
        m_StaticGeometryCmd->ShaderObject()->Set("view", m_Camera->GetView());
        m_StaticGeometryCmd->ShaderObject()->Set("projection", m_Camera->GetProjection());

        const auto* meshes = draw_info.Model->GetMeshesPtr();

        uint32_t index_inner_offset_bytes = 0;
        for (const auto& mesh : *meshes)
        {
            for (uint32_t i = 0; i < mesh.PrimitivesCount(); i++)
            {
                const auto primitive = mesh.Primitives() + i;
                m_StaticGeometryCmd->PushDebugGroup(0, mesh.Name().data());
                m_StaticGeometryCmd->ShaderObject()->BindMaterial(draw_info.Model->GetMaterial(primitive->MaterialIdx()));
                m_StaticGeometryCmd->IndexedDraw(primitive->IndexCount(), draw_info.IndexGlobalOffsetBytes + index_inner_offset_bytes,
                                                 draw_info.VertexGlobalOffsetBytes / sizeof(VertexData));

                index_inner_offset_bytes += primitive->IndexSize();
                m_StaticGeometryCmd->PopDebugGroup();
            }
        }
        m_StaticGeometryCmd->PopDebugGroup();
        m_StaticGeometryCmd->EndRecording();
        m_StaticGeometryCmd->Submit();
    }
    m_StaticGeometryCmd->PopDebugGroup();
}

}  // namespace Fleur::Graphics
