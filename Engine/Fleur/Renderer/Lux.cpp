#include "Lux.h"

#include "Application.h"
#include "AssetsManager.h"
#include "Image2D.h"
#include "Services/ServiceLocator.h"
#include "Shader.h"
#include "Vulkan/Renderer_Vulkan.h"  // vk::backend

namespace Lux
{
Renderer::~Renderer()
{
    OnShutdown();
}

void Renderer::OnInit()
{
    Fleur::Application& application = Fleur::Application::instance();
    auto assetsManager = Fleur::ServiceLocator::instance().GetService<Fleur::AssetsManager>();

    auto fallbackAsset = assetsManager->Get<Fleur::Graphics::Image2D>("wall_placeholder2.png");
    Fleur::Graphics::SFLImageView fallbackView = fallbackAsset.obj->GetView();
    fallbackView.ID = fallbackAsset.handle.id;

    Fleur::SRect framebufferSize = application.GetWindow().GetFramebufferSize();

    bool validation = true;
    m_Backend = new vk::backend(validation, application.GetWindow().GetNativeHandle(), framebufferSize, fallbackView);

    m_Backend->CreatePass(Fleur::Graphics::EFLPassKind::Opaque,
                          {shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("opaqueVertex").obj),
                           shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("opaqueFragment").obj)});

    m_Backend->CreatePass(Fleur::Graphics::EFLPassKind::Transparent,
                          {shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("transparentVertex").obj),
                           shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("transparentFragment").obj)});

    m_Backend->CreateSkybox(fallbackAsset.handle.id,
                            {shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("skyboxVertex").obj),
                             shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("skyboxFragment").obj)});
}

void Renderer::OnShutdown()
{
    delete m_Backend;
    m_Backend = nullptr;
}

Fleur::Graphics::SFLShaderInfo Renderer::shaderInfo(Fleur::Graphics::Shader* shader)
{
    if (!shader)
        return Fleur::Graphics::SFLShaderInfo();

    return {shader->GetShaderCode(), shader->GetShaderCodeSizeB()};
}

void Renderer::Register(AssetID model, const Fleur::Graphics::SVertexData* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount,
                        const Fleur::Graphics::FLDrawItem* primitives, uint32_t primitiveCount)
{
    m_Backend->RegisterModel(model, vertices, vertexCount, indices, indexCount, primitives, primitiveCount);
}
void Renderer::Unregister(AssetID model)
{
    m_Backend->UnregisterModel(model);
}
void Renderer::UploadTextures(Fleur::Graphics::SFLImageViewInfo* info)
{
    m_Backend->SubmitImageViews(info);
}
void Renderer::RemoveTexture(AssetID texture)
{
    m_Backend->RemoveTexture(texture);
}

void Renderer::BeginFrame(const CameraView& camera)
{
    Fleur::Graphics::SFLCameraData data{camera.dir, camera.view, camera.proj};
    m_Backend->BeginFrame(data);
}
void Renderer::Draw(AssetID model, const glm::mat4& transform)
{
    m_Backend->Draw(model, transform);
}
void Renderer::EndFrame()
{
    m_Backend->EndFrame();
}

void Renderer::StartResize()
{
    m_Backend->StartResize();
}
void Renderer::EndResize(Fleur::SRect& rect)
{
    m_Backend->EndResize(rect);
}
void Renderer::SetSkybox(AssetID id)
{
    m_Backend->SetSkybox(id);
}
}  // namespace Lux
