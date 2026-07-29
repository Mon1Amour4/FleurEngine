#pragma once

#if defined(FLEUR_PLATFORM_WIN)
#define NOMINMAX
#include <windows.h>  // GDI needed: GetDC / ChoosePixelFormat / SetPixelFormat / SwapBuffers
#endif

#include <glad/gl.h>
#include <glad/wgl.h>

#include <Fleur/Math/Math.hpp>
#include <unordered_map>
#include <vector>

#include "Renderer_OpenGL.h"

namespace gl
{
// One registered model = its primitives, each a sub-range of the shared index buffer.
struct GlPrimitive
{
    uint32_t albedo;
    uint32_t indexCount;
    uint32_t indexOffset;  // in indices
    int32_t vertexOffset;  // in vertices (baseVertex)
};

struct backend::impl
{
    impl(bool enableValidation, void* pNativeHandle, Fleur::SRect& framebufferSize, SFLImageView& fallback, uint32_t maxPointLights);
    ~impl();

    // ---------- context (mirrors FVkDevice) ----------
    HWND m_Hwnd{nullptr};
    HDC m_Hdc{nullptr};
    HGLRC m_Ctx{nullptr};
    void createContext(void* pNativeHandle);

    int m_Width{0};
    int m_Height{0};
    uint32_t m_MaxPointLights{0};

    // ---------- frame ----------
    Fleur::Mat4 m_View{1.0f};
    Fleur::Mat4 m_Proj{1.0f};
    Fleur::Vec3 m_CameraDir{0.0f};
    void beginFrame(const RenderFrameData& frameData);
    void draw(AssetID model, const Fleur::Mat4& transform);
    void endFrame();

    // ---------- resources ----------
    std::unordered_map<AssetID, std::vector<GlPrimitive>> m_RegisteredModels;
    void registerModel(AssetID id, const SVertexData* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount,
                       const FLPrimitiveDrawItem* primitives, uint32_t primitiveCount);
    void unregisterModel(AssetID id);

    std::unordered_map<AssetID, GLuint> m_TextureMap;
    AssetID m_FallbackTexture{0};
    GLuint createTexture(const SFLImageView& view);
    void uploadTextures(SFLImageViewInfo* pInfo);
    void removeTexture(AssetID id);

    // ---------- geometry pass ----------
    GLuint m_GeometryProgram{0};
    GLuint m_Vao{0};
    GLuint m_Vbo{0};
    GLuint m_Ebo{0};
    uint32_t m_VertexCursor{0};  // bump cursor into the shared VBO (in vertices)
    uint32_t m_IndexCursor{0};   // bump cursor into the shared EBO (in indices)
    void createGeometry();

    // ---------- passes ----------
    void createPass(EFLPassKind kind, SFLShaderStages shaderStages);

    // ---------- skybox ----------
    GLuint m_SkyboxProgram{0};
    GLuint m_SkyboxVao{0};
    GLuint m_SkyboxVbo{0};
    GLuint m_SkyboxCubemap{0};
    GLuint createCubemap(const SFLImageView& view);
    void createSkybox(AssetID id, SFLShaderStages shaderStages);
    void setSkybox(AssetID id);
    void renderSkybox();
};
}  // namespace gl
