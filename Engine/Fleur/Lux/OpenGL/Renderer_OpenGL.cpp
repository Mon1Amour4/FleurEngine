#include <cstddef>
#include <iostream>

#include "PrivateOpenGLImpl.hpp"

namespace
{
constexpr GLsizeiptr kVertexBufferBytes = 64ull * 1024 * 1024;
constexpr GLsizeiptr kIndexBufferBytes = 16ull * 1024 * 1024;

// GL uses its own GLSL: the engine ships Vulkan-targeted SPIR-V, which GL can't consume directly.
const char* kGeometryVS = R"(#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTex;
layout(location = 2) in vec3 aNormal;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
out vec2 vTex;
void main()
{
    vTex = aTex;
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}
)";

const char* kGeometryFS = R"(#version 460 core
in vec2 vTex;
out vec4 FragColor;
uniform sampler2D uAlbedo;
void main()
{
    FragColor = texture(uAlbedo, vTex);
}
)";

const char* kSkyboxVS = R"(#version 460 core
layout(location = 0) in vec3 aPos;
uniform mat4 uView;
uniform mat4 uProj;
out vec3 vDir;
void main()
{
    vDir = aPos;
    vec4 pos = uProj * uView * vec4(aPos, 1.0);
    gl_Position = pos.xyww;  // force depth = 1 (behind everything)
}
)";

const char* kSkyboxFS = R"(#version 460 core
in vec3 vDir;
out vec4 FragColor;
uniform samplerCube uSkybox;
void main()
{
    FragColor = texture(uSkybox, vDir);
}
)";

// Unit cube, 36 verts (matches the Vulkan skybox).
const float kSkyboxVerts[] = {-1, 1,  -1, -1, -1, -1, 1,  -1, -1, 1,  -1, -1, 1,  1,  -1, -1, 1,  -1,  //
                              -1, -1, 1,  -1, -1, -1, -1, 1,  -1, -1, 1,  -1, -1, 1,  1,  -1, -1, 1,   //
                              1,  -1, -1, 1,  -1, 1,  1,  1,  1,  1,  1,  1,  1,  1,  -1, 1,  -1, -1,  //
                              -1, -1, 1,  -1, 1,  1,  1,  1,  1,  1,  1,  1,  1,  -1, 1,  -1, -1, 1,   //
                              -1, 1,  -1, 1,  1,  -1, 1,  1,  1,  1,  1,  1,  -1, 1,  1,  -1, 1,  -1,  //
                              -1, -1, -1, -1, -1, 1,  1,  -1, -1, 1,  -1, -1, -1, -1, 1,  1,  -1, 1};

GLuint compileShader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cout << "[OpenGL] shader compile: " << log << '\n';
        assert(false);
    }
    return shader;
}

GLuint compileProgram(const char* vs, const char* fs)
{
    GLuint v = compileShader(GL_VERTEX_SHADER, vs);
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fs);
    GLuint program = glCreateProgram();
    glAttachShader(program, v);
    glAttachShader(program, f);
    glLinkProgram(program);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cout << "[OpenGL] program link: " << log << '\n';
        assert(false);
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return program;
}
}  // namespace

// ---------- backend (public shim) ----------
gl::backend::backend(bool enableValidation, void* pNativeHandle, Fleur::SRect& framebufferSize, SFLImageView& fallback, uint32_t maxPointLights)
    : pImpl(new gl::backend::impl(enableValidation, pNativeHandle, framebufferSize, fallback, maxPointLights))
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
void gl::backend::ConfigureDebugDraw(const SFLDebugDrawShaders& shaders)
{
    (void)shaders;  // TODO: GlDebugDraw
}
void gl::backend::ConfigureOverlay(SFLShaderStages shaderStages)
{
    (void)shaderStages;  // TODO: GlOverlay
}
void gl::backend::CreateSkybox(AssetID id, SFLShaderStages shaderStages)
{
    pImpl->createSkybox(id, shaderStages);
}
void gl::backend::SetSkybox(AssetID id)
{
    pImpl->setSkybox(id);
}
void gl::backend::CreateFloor(AssetID texture, SFLShaderStages shaderStages, float height)
{
    (void)texture;
    (void)shaderStages;
    (void)height;
}
void gl::backend::SetFloor(AssetID texture, float height)
{
    (void)texture;
    (void)height;
}
void gl::backend::RegisterModel(const SFLModelRegisterInfo& info)
{
    pImpl->registerModel(info.model, info.vertices, info.vertexCount, info.indices, info.indexCount, info.primitives, info.primitiveCount);
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
void gl::backend::BeginFrame(const RenderFrameData& frameData)
{
    pImpl->beginFrame(frameData);
}
void gl::backend::Draw(AssetID model, const Fleur::Mat4& transform)
{
    pImpl->draw(model, transform);
}
void gl::backend::EndFrame()
{
    pImpl->endFrame();
}
void gl::backend::StartResize()
{
}
void gl::backend::EndResize(Fleur::SRect& rect)
{
    (void)rect;  // TODO: glViewport to new size (next stage)
}

// ---------- impl ----------
gl::backend::impl::impl(bool /*enableValidation*/, void* pNativeHandle, Fleur::SRect& framebufferSize, SFLImageView& fallback, uint32_t maxPointLights)
{
    m_MaxPointLights = maxPointLights;
    m_Width = framebufferSize.width;
    m_Height = framebufferSize.height;
    m_FallbackTexture = fallback.ID;

    createContext(pNativeHandle);

    glViewport(0, 0, m_Width, m_Height);
    glEnable(GL_DEPTH_TEST);

    createGeometry();

    m_TextureMap[m_FallbackTexture] = createTexture(fallback);
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

    const int attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB,    4, WGL_CONTEXT_MINOR_VERSION_ARB, 6, WGL_CONTEXT_PROFILE_MASK_ARB,
                           WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0};
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

// ---------- geometry ----------
void gl::backend::impl::createGeometry()
{
    glGenVertexArrays(1, &m_Vao);
    glGenBuffers(1, &m_Vbo);
    glGenBuffers(1, &m_Ebo);

    glBindVertexArray(m_Vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
    glBufferData(GL_ARRAY_BUFFER, kVertexBufferBytes, nullptr, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, kIndexBufferBytes, nullptr, GL_STATIC_DRAW);

    // SVertexData: vec3 Position | vec2 TexCoord | vec3 Normal (packed)
    const GLsizei stride = sizeof(SVertexData);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SVertexData, Position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SVertexData, TexCoord));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SVertexData, Normal));

    glBindVertexArray(0);
}

void gl::backend::impl::createPass(EFLPassKind kind, SFLShaderStages /*shaderStages*/)
{
    if (kind == EFLPassKind::Geometry)
        m_GeometryProgram = compileProgram(kGeometryVS, kGeometryFS);
}

void gl::backend::impl::registerModel(AssetID id, const SVertexData* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount,
                                      const FLPrimitiveDrawItem* primitives, uint32_t primitiveCount)
{
    const uint32_t baseVertex = m_VertexCursor;
    const uint32_t baseIndex = m_IndexCursor;

    glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
    glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)baseVertex * sizeof(SVertexData), (GLsizeiptr)vertexCount * sizeof(SVertexData), vertices);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (GLintptr)baseIndex * sizeof(uint32_t), (GLsizeiptr)indexCount * sizeof(uint32_t), indices);

    m_VertexCursor += vertexCount;
    m_IndexCursor += indexCount;

    auto& list = m_RegisteredModels[id];
    for (uint32_t i = 0; i < primitiveCount; i++)
    {
        const auto& item = primitives[i];
        GlPrimitive prim{};
        prim.albedo = item.material.albedo;
        prim.indexCount = item.indexCount;
        prim.indexOffset = baseIndex + item.indexStart;
        prim.vertexOffset = static_cast<int32_t>(baseVertex);
        list.push_back(prim);
    }
}

// ---------- frame ----------
void gl::backend::impl::beginFrame(const RenderFrameData& frameData)
{
    const auto& cameraData = frameData.camera;
    m_View = cameraData.view;
    m_Proj = cameraData.proj;
    m_CameraDir = cameraData.cameraDir;

    glViewport(0, 0, m_Width, m_Height);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderSkybox();

    if (!m_GeometryProgram)
        return;

    glUseProgram(m_GeometryProgram);
    glUniformMatrix4fv(glGetUniformLocation(m_GeometryProgram, "uView"), 1, GL_FALSE, &m_View[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_GeometryProgram, "uProj"), 1, GL_FALSE, &m_Proj[0][0]);
    glUniform1i(glGetUniformLocation(m_GeometryProgram, "uAlbedo"), 0);
    glBindVertexArray(m_Vao);
}

void gl::backend::impl::draw(AssetID model, const Fleur::Mat4& transform)
{
    if (!m_GeometryProgram)
        return;

    auto it = m_RegisteredModels.find(model);
    if (it == m_RegisteredModels.end())
        return;

    glUniformMatrix4fv(glGetUniformLocation(m_GeometryProgram, "uModel"), 1, GL_FALSE, &transform[0][0]);

    for (const auto& prim : it->second)
    {
        auto tex = m_TextureMap.find(prim.albedo);
        GLuint glTexture = (tex != m_TextureMap.end()) ? tex->second : m_TextureMap[m_FallbackTexture];
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, glTexture);
        glDrawElementsBaseVertex(GL_TRIANGLES, prim.indexCount, GL_UNSIGNED_INT, (void*)((GLintptr)prim.indexOffset * sizeof(uint32_t)), prim.vertexOffset);
    }
}

void gl::backend::impl::endFrame()
{
    SwapBuffers(m_Hdc);
}

// ---------- stubs (next stages: textures -> skybox) ----------
void gl::backend::impl::unregisterModel(AssetID id)
{
    m_RegisteredModels.erase(id);
}
GLuint gl::backend::impl::createTexture(const SFLImageView& view)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    GLenum format = (view.channels == 4) ? GL_RGBA : (view.channels == 3) ? GL_RGB : GL_RED;
    GLint internalFormat = (view.channels == 4) ? GL_RGBA8 : (view.channels == 3) ? GL_RGB8 : GL_R8;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, view.w, view.h, 0, format, GL_UNSIGNED_BYTE, view.pData);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return texture;
}

void gl::backend::impl::uploadTextures(SFLImageViewInfo* pInfo)
{
    for (uint32_t i = 0; i < pInfo->count; i++)
    {
        const SFLImageView& view = pInfo->pData[i];
        if (m_TextureMap.contains(view.ID))
            continue;
        m_TextureMap[view.ID] = (view.layerCount == 6) ? createCubemap(view) : createTexture(view);
    }
}
void gl::backend::impl::removeTexture(AssetID id)
{
    m_TextureMap.erase(id);
}
GLuint gl::backend::impl::createCubemap(const SFLImageView& view)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    GLenum format = (view.channels == 4) ? GL_RGBA : (view.channels == 3) ? GL_RGB : GL_RED;
    GLint internalFormat = (view.channels == 4) ? GL_RGBA8 : (view.channels == 3) ? GL_RGB8 : GL_R8;
    const uint32_t faceBytes = view.w * view.h * view.channels;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (uint32_t face = 0; face < 6; face++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, internalFormat, view.w, view.h, 0, format, GL_UNSIGNED_BYTE, view.pData + faceBytes * face);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return texture;
}

void gl::backend::impl::createSkybox(AssetID id, SFLShaderStages /*shaderStages*/)
{
    m_SkyboxProgram = compileProgram(kSkyboxVS, kSkyboxFS);

    glGenVertexArrays(1, &m_SkyboxVao);
    glGenBuffers(1, &m_SkyboxVbo);
    glBindVertexArray(m_SkyboxVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_SkyboxVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kSkyboxVerts), kSkyboxVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
    glBindVertexArray(0);

    setSkybox(id);
}

void gl::backend::impl::setSkybox(AssetID id)
{
    auto it = m_TextureMap.find(id);
    if (it != m_TextureMap.end())
        m_SkyboxCubemap = it->second;
}

void gl::backend::impl::renderSkybox()
{
    if (!m_SkyboxProgram || !m_SkyboxCubemap)
        return;

    glDepthFunc(GL_LEQUAL);
    glUseProgram(m_SkyboxProgram);

    Fleur::Mat4 viewNoTranslation = Fleur::Mat4(Fleur::Mat3(m_View));  // strip camera translation
    glUniformMatrix4fv(glGetUniformLocation(m_SkyboxProgram, "uView"), 1, GL_FALSE, &viewNoTranslation[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_SkyboxProgram, "uProj"), 1, GL_FALSE, &m_Proj[0][0]);
    glUniform1i(glGetUniformLocation(m_SkyboxProgram, "uSkybox"), 0);

    glBindVertexArray(m_SkyboxVao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyboxCubemap);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDepthFunc(GL_LESS);
}
