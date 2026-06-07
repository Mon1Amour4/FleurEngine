#include "PrivateOpenGLImpl.hpp"

#include <iostream>

// ---------- backend (public shim) ----------
gl::backend::backend(bool enableValidation, void* pNativeHandle, Fleur::SRect& framebufferSize, SFLImageView& fallback)
    : pImpl(new gl::backend::impl(enableValidation, pNativeHandle, framebufferSize, fallback))
{
}
gl::backend::~backend()
{
    delete pImpl;
}

void gl::backend::CreatePass(EFLPassKind kind, SFLShaderStages shaderStages)
{
    pImpl->createPass(kind, shaderStages);
}
void gl::backend::CreateSkybox(AssetID id, SFLShaderStages shaderStages)
{
    pImpl->createSkybox(id, shaderStages);
}
void gl::backend::SetSkybox(AssetID id)
{
    pImpl->setSkybox(id);
}
void gl::backend::RegisterModel(AssetID model, const SVertexData* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount,
                                const FLDrawItem* primitives, uint32_t primitiveCount)
{
    pImpl->registerModel(model, vertices, vertexCount, indices, indexCount, primitives, primitiveCount);
}
void gl::backend::UnregisterModel(AssetID model)
{
    pImpl->unregisterModel(model);
}
void gl::backend::UploadTextures(SFLImageViewInfo* pInfo)
{
    pImpl->uploadTextures(pInfo);
}
void gl::backend::RemoveTexture(AssetID texture)
{
    pImpl->removeTexture(texture);
}
void gl::backend::BeginFrame(SFLCameraData& cameraData)
{
    pImpl->beginFrame(cameraData);
}
void gl::backend::Draw(AssetID model, const glm::mat4& transform)
{
    pImpl->draw(model, transform);
}
void gl::backend::EndFrame()
{
    pImpl->endFrame();
}
void gl::backend::StartResize() {}
void gl::backend::EndResize(Fleur::SRect& rect)
{
    (void)rect;  // TODO: glViewport to new size (next stage)
}

// ---------- impl ----------
gl::backend::impl::impl(bool /*enableValidation*/, void* pNativeHandle, Fleur::SRect& framebufferSize, SFLImageView& fallback)
{
    m_Width = framebufferSize.width;
    m_Height = framebufferSize.height;
    m_FallbackTexture = fallback.ID;

    createContext(pNativeHandle);

    glViewport(0, 0, m_Width, m_Height);
    glEnable(GL_DEPTH_TEST);
}

gl::backend::impl::~impl()
{
    if (m_Ctx)
    {
        wglMakeCurrent(m_Hdc, nullptr);
        wglDeleteContext(m_Ctx);
    }
    if (m_Hdc && m_Hwnd)
        ReleaseDC(m_Hwnd, m_Hdc);
}

void gl::backend::impl::createContext(void* pNativeHandle)
{
    m_Hwnd = reinterpret_cast<HWND>(pNativeHandle);
    m_Hdc = GetDC(m_Hwnd);

    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;

    int pixelFormat = ChoosePixelFormat(m_Hdc, &pfd);
    SetPixelFormat(m_Hdc, pixelFormat, &pfd);

    // Bootstrap a legacy context to load WGL extensions, then create a core 4.6 context.
    HGLRC dummy = wglCreateContext(m_Hdc);
    wglMakeCurrent(m_Hdc, dummy);

    if (!gladLoaderLoadWGL(m_Hdc))
    {
        std::cout << "[OpenGL] Failed to load WGL\n";
        assert(false);
    }

    const int attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 4, WGL_CONTEXT_MINOR_VERSION_ARB,        6,
                           WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0};
    m_Ctx = wglCreateContextAttribsARB(m_Hdc, nullptr, attribs);

    wglMakeCurrent(m_Hdc, m_Ctx);
    wglDeleteContext(dummy);

    if (!gladLoaderLoadGL())
    {
        std::cout << "[OpenGL] Failed to load GL\n";
        assert(false);
    }

    std::cout << "[OpenGL] " << glGetString(GL_VERSION) << '\n';
}

// ---------- frame ----------
void gl::backend::impl::beginFrame(SFLCameraData& cameraData)
{
    m_View = cameraData.view;
    m_Proj = cameraData.proj;
    m_CameraDir = cameraData.cameraDir;

    glViewport(0, 0, m_Width, m_Height);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void gl::backend::impl::endFrame()
{
    SwapBuffers(m_Hdc);
}

// ---------- stubs (next stages: shaders -> geometry -> textures -> skybox) ----------
void gl::backend::impl::draw(AssetID /*model*/, const glm::mat4& /*transform*/) {}
void gl::backend::impl::registerModel(AssetID /*id*/, const SVertexData* /*vertices*/, uint32_t /*vertexCount*/, const uint32_t* /*indices*/,
                                      uint32_t /*indexCount*/, const FLDrawItem* /*primitives*/, uint32_t /*primitiveCount*/)
{
}
void gl::backend::impl::unregisterModel(AssetID id)
{
    m_RegisteredModels.erase(id);
}
void gl::backend::impl::uploadTextures(SFLImageViewInfo* /*pInfo*/) {}
void gl::backend::impl::removeTexture(AssetID id)
{
    m_TextureMap.erase(id);
}
void gl::backend::impl::createPass(EFLPassKind /*kind*/, SFLShaderStages /*shaderStages*/) {}
void gl::backend::impl::createSkybox(AssetID /*id*/, SFLShaderStages /*shaderStages*/) {}
void gl::backend::impl::setSkybox(AssetID /*id*/) {}
