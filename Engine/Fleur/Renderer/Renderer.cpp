#include "Renderer.h"

#include <span>

#include "FleurAllocator.hpp"

// Backend
#include "Vulkan/Renderer_Vulkan.h"

uint32_t Fleur::Graphics::Renderer::MAX_TEXTURES_COUNT = 0;

Fleur::Graphics::Renderer::Renderer(EGraphicsAPI api)
    : m_ShowWireframe(false)
    , m_Camera(nullptr)
    , m_IsVsync(true)
    , m_Backend(nullptr)
{
}

Fleur::Graphics::Renderer::~Renderer()
{
    Fleur::Memory::FleurAllocator<Camera> alloc;
    alloc.deallocate(m_Camera, 1);
    m_Camera = nullptr;
}

std::shared_ptr<Fleur::Graphics::Texture> Fleur::Graphics::Renderer::Load_Texture(std::string_view path)
{
    // if (path.empty())
    //     return GetLoadedTexture("fallback");

    //// Try to find first across loaded textures:
    // std::string name = std::filesystem::path(path.data()).stem().string();
    // auto it = m_Textures.find(name);
    // if (it != m_Textures.end())
    //     return it->second;

    //// If not, load Image and then create new texture:
    // std::shared_ptr<Fleur::Graphics::Image2D> image{nullptr};
    // auto assetsManager = ServiceLocator::instance().GetService<AssetsManager>();
    // auto existingImg = assetsManager->Get<Fleur::Graphics::Image2D>(name);
    // if (!existingImg.expired())
    //     image = existingImg.lock();
    // else
    //{
    //     image = assetsManager->Load<Fleur::Graphics::Image2D>(path)->Resource();
    //     if (!image.get())
    //     {
    //         return GetLoadedTexture("fallback");
    //     }
    // }

    // auto texture = m_Toolchain->LoadTexture(image, m_Device.get());
    // return m_Textures.emplace(image->GetName(), texture).first->second;
    return std::shared_ptr<Fleur::Graphics::Texture>();
}

std::shared_ptr<Fleur::Graphics::Texture> Fleur::Graphics::Renderer::Load_Texture(std::string_view name, Color color, int width, int height)
{
    if (name.empty())
        return GetLoadedTexture("fallback");

    // Try to find first across loaded textures:
    // auto it = m_Textures.find(name.data());
    // if (it != m_Textures.end())
    //    return it->second;

    //// Create image
    // std::shared_ptr<Fleur::Graphics::Image2D> image{nullptr};
    // auto assetsManager = ServiceLocator::instance().GetService<AssetsManager>();
    // auto existingImg = assetsManager->Get<Fleur::Graphics::Image2D>(name);
    // if (!existingImg.expired())
    //     image = existingImg.lock();
    // else
    //     image = assetsManager->LoadImage2DFromColor(name, color, width, height)->Resource();

    // auto texture = m_Toolchain->LoadTexture(image, m_Device.get());
    // return m_Textures.emplace(image->GetName(), texture).first->second;
}

std::shared_ptr<Fleur::Graphics::Texture> Fleur::Graphics::Renderer::Load_Texture(std::shared_ptr<Fleur::Graphics::Image2D> img)
{
    if (!img)
        return GetLoadedTexture("fallback");

    std::string name = std::filesystem::path(img->GetName()).stem().string();
    auto it = m_Textures.find(name);
    if (it != m_Textures.end())
        return it->second;

    auto texture = m_Toolchain->LoadTexture(img, m_Device.get());
    return m_Textures.emplace(img->GetName(), texture).first->second;
}

void Fleur::Graphics::Renderer::OnInit()
{
    Fleur::Memory::FleurAllocator<Camera> alloc;
    m_Camera = alloc.construct_at();
    m_Camera->Activate();

    Fleur::Application& application = Fleur::Application::instance();

    auto assetsManager = Fleur::ServiceLocator::instance().GetService<Fleur::AssetsManager>();

    auto fallbackAsset = assetsManager->Get<Fleur::Graphics::Image2D>("wall_placeholder2.png");
    Fleur::Graphics::SFLImageView fallbackView = fallbackAsset.obj->GetView();
    fallbackView.ID = fallbackAsset.handle.id;

    Fleur::SRect framebufferSize = application.GetWindow().GetFramebufferSize();

    bool validation = true;
    m_Backend = new vk::backend(validation, application.GetWindow().GetNativeHandle(), framebufferSize, fallbackView);

    m_Backend->CreatePass(EFLPassKind::Opaque,
                          {GetShaderInfo(assetsManager->Get<Shader>("opaqueVertex").obj), GetShaderInfo(assetsManager->Get<Shader>("opaqueFragment").obj)});

    m_Backend->CreatePass(EFLPassKind::Transparent, {GetShaderInfo(assetsManager->Get<Shader>("transparentVertex").obj),
                                                     GetShaderInfo(assetsManager->Get<Shader>("transparentFragment").obj)});

    m_Backend->CreateSkybox(fallbackAsset.handle.id,
                            {GetShaderInfo(assetsManager->Get<Shader>("skyboxVertex").obj), GetShaderInfo(assetsManager->Get<Shader>("skyboxFragment").obj)});
}

void Fleur::Graphics::Renderer::OnShutdown()
{
    delete m_Backend;
}

void Fleur::Graphics::Renderer::StartResize()
{
    m_Backend->StartResize();
}

void Fleur::Graphics::Renderer::EndResize(Fleur::SRect& rect)
{
    m_Backend->EndResize(rect);
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
    std::vector<FLDrawItem> items;
    items.reserve(model->GetPrimitiveCount());

    for (size_t i = 0; i < model->GetMeshCount(); i++)
    {
        const auto& mesh = model->GetMeshData() + i;
        for (size_t i = 0; i < mesh->GetPrimitiveCount(); i++)
        {
            const auto& primitive = mesh->GetPrimitives() + i;

            auto& it = items.emplace_back();
            it.albedoId = model->GetMaterialsData()[primitive->GetMaterialIdx()].albedo;
            it.indexCount = primitive->GetIdxCount();
            it.indexStart = primitive->GetIdxStart();
            it.vertexStart = primitive->GetVertexStart();
            it.bucket = primitive->GetAlphaMode();
        }
    }
    m_Backend->AddModel(model->GetVerticesData(), model->GetVertexCount(), model->GetIdxData(), model->GetIdxCount(), items.data(), items.size());
}

void Fleur::Graphics::Renderer::SetSkybox(AssetID id)
{
    m_Backend->SetSkybox(id);
}

void Fleur::Graphics::Renderer::Clear()
{
}

void Fleur::Graphics::Renderer::Present()
{
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
    Fleur::Graphics::SFLCameraData cameraData{m_Camera->GetCameraForward(), m_Camera->GetView(), m_Camera->GetProjection()};

    m_Backend->Update(cameraData);
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

void Fleur::Graphics::Renderer::SubmitImageViews(Fleur::Graphics::SFLImageViewInfo* pInfo)
{
    m_Backend->SubmitImageViews(pInfo);
}

void Fleur::Graphics::Renderer::UpdateViewport(Fleur::SRect& rect)
{
}

Fleur::Graphics::SVertexData::SVertexData(glm::vec3 pos, glm::vec3 texCoord, glm::vec3 normal)
    : Position(pos)
    , TexCoord(texCoord)
    , Normal(normal)
{
}

SFLShaderInfo Fleur::Graphics::Renderer::GetShaderInfo(Fleur::Graphics::Shader* shader)
{
    if (!shader)
        return SFLShaderInfo();

    return {shader->GetShaderCode(), shader->GetShaderCodeSizeB()};
}
