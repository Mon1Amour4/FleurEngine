#include "Renderer.h"

#include <span>

#include "FleurAllocator.hpp"

// Backend
#include "Vulkan/Renderer_Vulkan.h"

uint32_t Fleur::Graphics::Renderer::MAX_TEXTURES_COUNT = 0;

Fleur::Graphics::Renderer::Renderer(EGraphicsAPI api, std::unique_ptr<Fleur::IRendererToolchain> toolchain)
    : m_ShowWireframe(false)
    , m_Camera(nullptr)
    , m_CurrentShaderObj(nullptr)
    , m_IsVsync(true)
    , m_Renderer(api)
    , m_Toolchain(std::move(toolchain))
    , m_Backend(nullptr)
{
}

Fleur::Graphics::Renderer::~Renderer()
{
    OnShutdown();
}

std::shared_ptr<Fleur::Graphics::Texture> Fleur::Graphics::Renderer::Load_Texture(std::string_view path)
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
    auto assetsManager = ServiceLocator::instance().GetService<AssetsManager>();
    auto existingImg = assetsManager->Get<Fleur::Graphics::Image2D>(name);
    if (!existingImg.expired())
        image = existingImg.lock();
    else
    {
        image = assetsManager->Load<Fleur::Graphics::Image2D>(path)->Resource();
        if (!image.get())
        {
            return GetLoadedTexture("fallback");
        }
    }

    auto texture = m_Toolchain->LoadTexture(image, m_Device.get());
    return m_Textures.emplace(image->Name(), texture).first->second;
}

std::shared_ptr<Fleur::Graphics::Texture> Fleur::Graphics::Renderer::Load_Texture(std::string_view name, Color color, int width, int height)
{
    if (name.empty())
        return GetLoadedTexture("fallback");

    // Try to find first across loaded textures:
    auto it = m_Textures.find(name.data());
    if (it != m_Textures.end())
        return it->second;

    // Create image
    std::shared_ptr<Fleur::Graphics::Image2D> image{nullptr};
    auto assetsManager = ServiceLocator::instance().GetService<AssetsManager>();
    auto existingImg = assetsManager->Get<Fleur::Graphics::Image2D>(name);
    if (!existingImg.expired())
        image = existingImg.lock();
    else
        image = assetsManager->LoadImage2DFromColor(name, color, width, height)->Resource();

    auto texture = m_Toolchain->LoadTexture(image, m_Device.get());
    return m_Textures.emplace(image->Name(), texture).first->second;
}

std::shared_ptr<Fleur::Graphics::Texture> Fleur::Graphics::Renderer::Load_Texture(std::shared_ptr<Fleur::Graphics::Image2D> img)
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

void Fleur::Graphics::Renderer::OnInit()
{
    Fleur::Memory::FleurAllocator<Camera> alloc;
    m_Camera.reset(alloc.construct_at());
    m_Camera->Activate();
    //
    //    m_Device = Device::CreateDevice();
    //    m_Device->SetVSync(m_IsVsync);
    //
    //    m_CommandQueue = m_Device->CreateCommandQueue();
    //
    //    auto& application = Fleur::Application::instance();
    //
    //    m_Swapchain = m_Device->CreateSwapchain(m_Device->CreateSurface(application.GetWindow().GetNativeHandle()));
    //
    //    m_CommandPool = m_Device->CreateCommandPool(*m_CommandQueue);
    //
    //    auto staticGeoVs = m_Device->CreateShader("static_geo", Shader::EShaderType::Vertex);
    //    std::shared_ptr<ShaderObject> staticGeometryShader(
    //        ShaderObject::CreateShaderObject("static_geometry_shader", staticGeoVs, m_Device->CreateShader("static_geo", Shader::EShaderType::Pixel)));
    //    // Static geometry
    //    DepthStencilDescriptor staticGeoDescriptor{true, EDepthTestOperation::LESS};
    //    m_StaticGeometryCmd = m_Device->CreateCommandBuffer(staticGeoDescriptor);
    //    m_StaticGeometryCmd->BindShaderObject(staticGeometryShader);
    //
    //    VertexLayout layout{};
    //    layout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::EDataType::FLOAT, true));
    //    layout.AddAttribute(VertexLayout::VertexAttribute(1, 2, VertexLayout::EDataType::FLOAT, true));
    //    layout.AddAttribute(VertexLayout::VertexAttribute(2, 3, VertexLayout::EDataType::FLOAT, true));
    //    m_StaticGeometryCmd->BindVertexBuffer(m_Device->CreateBuffer(Fleur::Graphics::Buffer::EBufferType::Vertex, STATIC_GEOMETRY, 100 * 1024 * 1024),
    //    layout); m_StaticGeometryCmd->BindIndexBuffer(m_Device->CreateBuffer(Fleur::Graphics::Buffer::EBufferType::Index, STATIC_GEOMETRY, 100 * 1024 *
    //    1024));
    //
    //    // Skybox
    //    std::shared_ptr<ShaderObject> skyboxShader(ShaderObject::CreateShaderObject("skybox_shader", m_Device->CreateShader("skybox",
    //    Shader::EShaderType::Vertex),
    //                                                                                m_Device->CreateShader("skybox", Shader::EShaderType::Pixel)));
    //    DepthStencilDescriptor skyboxDescriptor{false, EDepthTestOperation::LESS_OR_EQUAL};
    //    m_SkyboxCmd = m_Device->CreateCommandBuffer(skyboxDescriptor);
    //    m_SkyboxCmd->BindShaderObject(skyboxShader);
    //
    //    VertexLayout skyboxLayout{};
    //    skyboxLayout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::EDataType::FLOAT, true));
    //    m_SkyboxCmd->BindVertexBuffer(m_Device->CreateBuffer(Fleur::Graphics::Buffer::EBufferType::Vertex, STATIC_GEOMETRY, 108 * sizeof(float)),
    //    skyboxLayout);
    //
    //    // gizmo
    //    std::shared_ptr<ShaderObject> gizmoShader(ShaderObject::CreateShaderObject(
    //        "gizmo_shader", m_Device->CreateShader("static_geo", Shader::EShaderType::Vertex), m_Device->CreateShader("gizmo", Shader::EShaderType::Pixel)));
    //    DepthStencilDescriptor gizmoDescriptor{false, EDepthTestOperation::ALWAYS};
    //    m_GizmoCmd = m_Device->CreateCommandBuffer(gizmoDescriptor);
    //    m_GizmoCmd->BindShaderObject(gizmoShader);
    //
    //    VertexLayout gizmoLayout{};
    //    gizmoLayout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::EDataType::FLOAT, true));
    //    gizmoLayout.AddAttribute(VertexLayout::VertexAttribute(1, 2, VertexLayout::EDataType::FLOAT, true));
    //    gizmoLayout.AddAttribute(VertexLayout::VertexAttribute(2, 3, VertexLayout::EDataType::FLOAT, true));
    //    m_GizmoCmd->BindVertexBuffer(m_Device->CreateBuffer(Fleur::Graphics::Buffer::EBufferType::Vertex, STATIC_GEOMETRY, 500 * 1024), gizmoLayout);
    //    m_GizmoCmd->BindIndexBuffer(m_Device->CreateBuffer(Fleur::Graphics::Buffer::EBufferType::Index, STATIC_GEOMETRY, 500 * 1024));
    //
    //    m_GizmoFBO = m_Device->CreateFramebuffer("gizmo_framebuffer", application.GetWindow().GetWidth(), application.GetWindow().GetHeight(),
    //                                             (uint32_t)EFramebufferSettings::COLOR | (uint32_t)EFramebufferSettings::DEPTH_STENCIL);
    //
    //
    //    std::shared_ptr<ShaderObject> copy_fbo_shader(
    //        ShaderObject::CreateShaderObject("copy_fbo_as_quad_shader", staticGeoVs, m_Device->CreateShader("CopyFBOAsQuad", Shader::EShaderType::Pixel)));
    //
    //    DepthStencilDescriptor copyFBODescriptor{true, EDepthTestOperation::LESS};
    //    m_CopyFBOCmd = m_Device->CreateCommandBuffer(copyFBODescriptor);
    //    m_CopyFBOCmd->BindShaderObject(copy_fbo_shader);
    //
    //    VertexLayout copyFBOLayout{};
    //    copyFBOLayout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::EDataType::FLOAT, true));
    //    copyFBOLayout.AddAttribute(VertexLayout::VertexAttribute(1, 2, VertexLayout::EDataType::FLOAT, true));
    //    m_CopyFBOCmd->BindVertexBuffer(m_Device->CreateBuffer(Fleur::Graphics::Buffer::EBufferType::Vertex, STATIC_GEOMETRY, 500 * 1024), copyFBOLayout);
    //    m_CopyFBOCmd->BindIndexBuffer(m_Device->CreateBuffer(Fleur::Graphics::Buffer::EBufferType::Index, STATIC_GEOMETRY, 500 * 1024));
    //
    //    // clang-format off
    //    static float quadVertices[] = {
    //    -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
    //     1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
    //    -1.0f,  1.0f, 0.0f,   0.0f, 1.0f,
    //     -1.0f, 1.0f, 0.0f,   1.0f, 1.0f,
    //     1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
    //     1.0f,  -1.0f, 0.0f,   1.0f, 1.0f
    //};
    //    // clang-format on
    //    m_CopyFBOCmd->UpdateBufferSubData<float>(Buffer::Vertex, std::span(quadVertices, 30));

    /*HMODULE backendLib = LoadLibraryA("Renderer_Vulkan_Backend");
    if (!backendLib)
    {
        std::cout << "Renderer_Vulkan_Backend invalid handle";
        DWORD error = GetLastError();
    }
    else
    {
        RendererBackend_t* CreateRendererBackend = (RendererBackend_t*)GetProcAddress(backendLib, "CreateRendererBackend");
        IRenderer* backend = CreateRendererBackend();
    }*/
    auto& application = Fleur::Application::instance();
    auto assetsManager = Fleur::ServiceLocator::instance().GetService<Fleur::AssetsManager>();

    auto pVertexShader = assetsManager->Get<Shader>("vertex").lock().get();
    Fleur::Graphics::SFLShaderInfo vertexShaderInfo{};
    vertexShaderInfo.shaderCode = pVertexShader->GetShaderCode();
    vertexShaderInfo.sizeBytes = pVertexShader->GetShaderCodeSizeB();

    auto pFragmentShader = assetsManager->Get<Shader>("opaque").lock().get();
    Fleur::Graphics::SFLShaderInfo fragmentShaderInfo{};
    fragmentShaderInfo.shaderCode = pFragmentShader->GetShaderCode();
    fragmentShaderInfo.sizeBytes = pFragmentShader->GetShaderCodeSizeB();

    Fleur::Graphics::SFLGeometryPass geometryPass{};
    geometryPass.pVertexShaderInfo = &vertexShaderInfo;
    geometryPass.pFragmentShaderInfo = &fragmentShaderInfo;
    geometryPass.vertexInputInfo = EFLVertexInputDescription::VERTEX_INPUT_VERTEX_DATA;
    geometryPass.indexInputInfo = EFLIndexInputDescription::INDEX_INPUT_UINT32;
    geometryPass.inputAssemblyTopology = EFLInputAssemblyTopology::FL_INPUT_ASSEMBLY_TOPOLOGY_TRIANGLE_LIST;

    Fleur::Graphics::SFLFrame frame{};
    frame.pPass = &geometryPass;

    m_Backend = new vulkanBackend(&frame, application.GetWindow().GetNativeHandle(), application.GetWindow().GetFramebufferSize());
}

void Fleur::Graphics::Renderer::OnShutdown()
{
    /* m_Device->Release();
     m_Swapchain->Release();*/

    delete m_Backend;
}

std::shared_ptr<Fleur::Graphics::Texture> Fleur::Graphics::Renderer::GetLoadedTexture(std::string_view path) const
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

void Fleur::Graphics::Renderer::DrawModel(ERenderStage stage, const Model* model, glm::mat4 model_pos)
{
    SFLDrawUploadInfo uploadInfo{};
    uploadInfo.pVertex = model->GetVerticesData();
    uploadInfo.vertexCount = model->GetVertexCount();
    uploadInfo.pIndex = model->GetIndicesData();
    uploadInfo.indexCount = model->GetIndicesCount();

    m_Backend->AddToDrawList(&uploadInfo);

    /* return;
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

         draw.IndexGlobalOffsetBytes = m_StaticGeometryCmd->UpdateBufferSubData(Buffer::Index, std::span(model->GetIndicesData(), model->GetIndicesCount()));

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
     }*/
}

void Fleur::Graphics::Renderer::Clear()
{
    // m_Swapchain->ClearBackbuffer();
    // m_GizmoFBO->Clear();
}

void Fleur::Graphics::Renderer::Present()
{
    // m_Swapchain->Present();
}

void Fleur::Graphics::Renderer::ShowWireFrame()
{
    if (m_ShowWireframe)
        m_Swapchain->ShowWireFrame(true);
    else
        m_Swapchain->ShowWireFrame(false);
}

void Fleur::Graphics::Renderer::ToggleWireFrame()
{
    m_ShowWireframe = !m_ShowWireframe;
}

void Fleur::Graphics::Renderer::ValidateWindow()
{
    // m_Swapchain->ValidateWindow();
}

void Fleur::Graphics::Renderer::SetVSync(bool active)
{
    m_IsVsync = active;
    // m_Device->SetVSync(m_IsVsync);
}

bool Fleur::Graphics::Renderer::IsVSync()
{
    return m_IsVsync;
}

void Fleur::Graphics::Renderer::OnUpdate(float dtTime)
{
    Fleur::Graphics::SFLGeometryUBO ubo{};
    ubo.view = m_Camera->GetView();
    ubo.proj = m_Camera->GetProjection();

    m_Backend->Update(&ubo);
    // auto assets = ServiceLocator::instance().GetService<AssetsManager>();
    // auto renderer = ServiceLocator::instance().GetService<Renderer>();
    //// ShaderComponentContext ctx{};
    // Fleur::Graphics::Texture* quadTexture = renderer->CreateGraphicsResource<Texture>("QuadTexture", Color(150, 150, 150, 255), 250, 250).get();
    // Fleur::Graphics::QuadRenderer quadRenderer = Fleur::Graphics::QuadRenderer(quadTexture);
    //// Fleur::Graphics::Model* quadModel = Model::QuadModel(Material::CreateMaterial());

    // UNUSED(dtTime);
    // m_Toolchain->Update();
    // static bool isSkyboxCreated = false;

    // auto assetsManager = ServiceLocator::instance().GetService<AssetsManager>();
    // static Fleur::Graphics::CubemapInitData skyboxImages;

    //{
    //    auto cubemap = assetsManager->Get<CubemapImage>("skybox_cross_layout_cubemap");
    //    if (!cubemap.expired() && !isSkyboxCreated)
    //    {
    //        auto cubemapTexture = m_Device->CreateCubemap(cubemap.lock().get());

    //        float skyboxVertices[] = {
    //            -1.0f, 1.0f,  -1.0f,  // 0
    //            -1.0f, -1.0f, -1.0f,  // 1
    //            1.0f,  -1.0f, -1.0f,  // 2
    //            1.0f,  -1.0f, -1.0f,  // 3
    //            1.0f,  1.0f,  -1.0f,  // 4
    //            -1.0f, 1.0f,  -1.0f,  // 5
    //            -1.0f, -1.0f, 1.0f,   // 6
    //            -1.0f, -1.0f, -1.0f,  // 7
    //            -1.0f, 1.0f,  -1.0f,  // 8
    //            -1.0f, 1.0f,  -1.0f,  // 9
    //            -1.0f, 1.0f,  1.0f,   // 10
    //            -1.0f, -1.0f, 1.0f,   // 11
    //            1.0f,  -1.0f, -1.0f,  // 12
    //            1.0f,  -1.0f, 1.0f,   // 13
    //            1.0f,  1.0f,  1.0f,   // 14
    //            1.0f,  1.0f,  1.0f,   // 15
    //            1.0f,  1.0f,  -1.0f,  // 16
    //            1.0f,  -1.0f, -1.0f,  // 17
    //            -1.0f, -1.0f, 1.0f,   // 18
    //            -1.0f, 1.0f,  1.0f,   // 19
    //            1.0f,  1.0f,  1.0f,   // 20
    //            1.0f,  1.0f,  1.0f,   // 21
    //            1.0f,  -1.0f, 1.0f,   // 22
    //            -1.0f, -1.0f, 1.0f,   // 23
    //            -1.0f, 1.0f,  -1.0f,  // 24
    //            1.0f,  1.0f,  -1.0f,  // 25
    //            1.0f,  1.0f,  1.0f,   // 26
    //            1.0f,  1.0f,  1.0f,   // 27
    //            -1.0f, 1.0f,  1.0f,   // 28
    //            -1.0f, 1.0f,  -1.0f,  // 29
    //            -1.0f, -1.0f, -1.0f,  // 30
    //            -1.0f, -1.0f, 1.0f,   // 31
    //            1.0f,  -1.0f, -1.0f,  // 32
    //            1.0f,  -1.0f, -1.0f,  // 33
    //            -1.0f, -1.0f, 1.0f,   // 34
    //            1.0f,  -1.0f, 1.0f    // 35
    //        };

    //        Fleur::Memory::FleurAllocator<Skybox> alloc;
    //        m_Skybox.reset(alloc.construct_at(cubemapTexture, std::span{skyboxVertices}));
    //        m_SkyboxCmd->UpdateBufferSubData<float>(Buffer::EBufferType::Vertex, std::span(m_Skybox->Data(), m_Skybox->GetVertexCount()));
    //        isSkyboxCreated = true;
    //    }
    //}

    // Skybox pass
    // SkyboxPass();

    // Main Pass
    // StaticGeometryPass();

    // gizmo
    // m_GizmoCmd->PushDebugGroup(0, "[STAGE] -> Gizmo");
    // m_GizmoCmd->BindRenderTarget(*m_GizmoFBO.get(), EFramebufferRWOperation::WRITE_ONLY);
    // m_GizmoCmd->BeginRecording();

    // m_GizmoCmd->ShaderObject()->Use();

    // for (const auto& draw_info : m_GizmoModelsVector)
    //{
    //     m_GizmoCmd->PushDebugGroup(0, draw_info.Model->GetName().data());

    //    glm::mat4 proj = glm::mat4(1.0f);

    //    glm::mat4 view = glm::mat4(1.f);

    //    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(-0.9f, -0.9f, 0.1f)), glm::vec3(0.05f, 0.05f, 0.05f));

    //    model = glm::rotate(model, glm::radians(m_Camera->Yaw()), glm::vec3(0.0f, 1.0f, 0.0f));
    //    model = glm::rotate(model, glm::radians(m_Camera->Pitch()), glm::vec3(1.0f, 0.0f, 0.0f));

    //    m_GizmoCmd->ShaderObject()->Set("projection", proj);
    //    m_GizmoCmd->ShaderObject()->Set("model", model);
    //    m_GizmoCmd->ShaderObject()->Set("view", view);

    //    const auto* meshes = draw_info.Model->GetMeshesPtr();

    //    uint32_t indexInnerOffsetBytes = 0;
    //    for (const auto& mesh : *meshes)
    //    {
    //        m_GizmoCmd->PushDebugGroup(0, mesh.Name().data());
    //        for (uint32_t i = 0; i < mesh.PrimitivesCount(); i++)
    //        {
    //            m_GizmoCmd->PushDebugGroup(0, "Primitive");
    //            auto primitive = mesh.Primitives() + i;
    //            m_GizmoCmd->ShaderObject()->BindMaterial(draw_info.Model->GetMaterial(primitive->MaterialIdx()));
    //            m_GizmoCmd->IndexedDraw(primitive->IndexCount(), draw_info.IndexGlobalOffsetBytes + indexInnerOffsetBytes,
    //                                    static_cast<uint32_t>(draw_info.VertexGlobalOffsetBytes / sizeof(VertexData)));

    //            indexInnerOffsetBytes += primitive->IndexSize();
    //            m_GizmoCmd->PopDebugGroup();
    //        }

    //        m_GizmoCmd->PopDebugGroup();
    //    }
    //    m_GizmoCmd->PopDebugGroup();
    //    m_GizmoCmd->EndRecording();
    //    m_GizmoCmd->Submit();
    //}
    // m_GizmoCmd->PopDebugGroup();

    // m_StaticGeometryCmd->BeginRecording();

    ////
    // m_CopyFBOCmd->PushDebugGroup(0, "[Copy FBO]");
    // m_GizmoCmd->BindRenderTarget(m_Swapchain->GetScreenTexture(), EFramebufferRWOperation::READ_WRITE);

    // ShaderComponentContext ctx{};
    // ctx.albedo_text.second = m_GizmoFBO->GetColorAttachment(0);

    // m_CopyFBOCmd->ShaderObject()->Use();
    // m_CopyFBOCmd->ShaderObject()->BindMaterial(Material::CreateMaterial(ctx));

    // m_CopyFBOCmd->ShaderObject()->Set("projection", glm::mat4(1.0f));
    // m_CopyFBOCmd->ShaderObject()->Set("model", glm::mat4(1.0f));
    // m_CopyFBOCmd->ShaderObject()->Set("view", glm::mat4(1.0f));

    // m_CopyFBOCmd->Draw(6);

    // m_CopyFBOCmd->PopDebugGroup();
}

void Fleur::Graphics::Renderer::OnPostUpdate(float dtTime)
{
    UNUSED(dtTime);
    // TODO
}

void Fleur::Graphics::Renderer::OnFixedUpdate()
{
    // TODO
}

void Fleur::Graphics::Renderer::UpdateViewport(Fleur::SRect& rect)
{
    // m_GizmoFBO->Bind();
    // uint32_t flags = m_GizmoFBO->Flags();
    // m_GizmoFBO.reset(m_Device->CreateFramebuffer("gizmo_fbo", width, height, flags).release());
    m_Backend->ResizeEvent(rect);
}

Fleur::Graphics::SVertexData::SVertexData(glm::vec3 pos, glm::vec3 texCoord, glm::vec3 normal)
    : Position(pos)
    , TexCoord(texCoord)
    , Normal(normal)
{
}

void Fleur::Graphics::Renderer::SkyboxPass() const
{
    if (!m_Skybox)
        return;

    m_SkyboxCmd->PushDebugGroup(0, "[STAGE] -> Skybox stage");
    m_SkyboxCmd->BeginRecording();
    m_SkyboxCmd->BindRenderTarget(m_Swapchain->GetScreenTexture(), EFramebufferRWOperation::READ_WRITE);

    m_SkyboxCmd->ShaderObject()->Use();

    m_SkyboxCmd->ShaderObject()->Set("view", m_Camera->GetView());
    m_SkyboxCmd->ShaderObject()->Set("projection", m_Camera->GetProjection());

    m_SkyboxCmd->ShaderObject()->BindMaterial(m_Skybox->GetMaterial());
    m_SkyboxCmd->Draw(m_Skybox->GetVertexCount() / 3);

    m_SkyboxCmd->PopDebugGroup();
}

void Fleur::Graphics::Renderer::StaticGeometryPass() const
{
    /*m_StaticGeometryCmd->PushDebugGroup(0, "[STAGE] -> Static geometry stage");
    m_StaticGeometryCmd->BindRenderTarget(m_Swapchain->GetScreenTexture(), EFramebufferRWOperation::READ_WRITE);
    m_StaticGeometryCmd->BeginRecording();

    m_StaticGeometryCmd->ShaderObject()->Use();

    for (const auto& draw_info : m_StaticGeometryModelsVector)
    {
        m_StaticGeometryCmd->PushDebugGroup(0, draw_info.Model->GetName().data());
        m_StaticGeometryCmd->ShaderObject()->Set("model", draw_info.ModelMatrix);
        m_StaticGeometryCmd->ShaderObject()->Set("view", m_Camera->GetView());
        m_StaticGeometryCmd->ShaderObject()->Set("projection", m_Camera->GetProjection());

        const auto* meshes = draw_info.Model->GetMeshesPtr();

        uint32_t indexInnerOffsetBytes = 0;
        for (const auto& mesh : *meshes)
        {
            for (uint32_t i = 0; i < mesh.PrimitivesCount(); i++)
            {
                const auto primitive = mesh.Primitives() + i;
                m_StaticGeometryCmd->PushDebugGroup(0, mesh.Name().data());
                m_StaticGeometryCmd->ShaderObject()->BindMaterial(draw_info.Model->GetMaterial(primitive->MaterialIdx()));
                m_StaticGeometryCmd->IndexedDraw(primitive->IndexCount(), draw_info.IndexGlobalOffsetBytes + indexInnerOffsetBytes,
                                                 static_cast<uint32_t>(draw_info.VertexGlobalOffsetBytes / sizeof(VertexData)));

                indexInnerOffsetBytes += primitive->IndexSize();
                m_StaticGeometryCmd->PopDebugGroup();
            }
        }
        m_StaticGeometryCmd->PopDebugGroup();
        m_StaticGeometryCmd->EndRecording();
        m_StaticGeometryCmd->Submit();
    }
    m_StaticGeometryCmd->PopDebugGroup();*/
}

Fleur::Graphics::QuadRenderer::QuadRenderer(const Texture* texture)
{
    ShaderComponentContext ctx{};
    ctx.albedo_text.second = static_cast<const Fleur::Graphics::Texture*>(texture);
    m_Material = Material::CreateMaterial(ctx);

    // m_Shader = ShaderObject::CreateShaderObject("QuadShader", "static_geo.frag", )
}
