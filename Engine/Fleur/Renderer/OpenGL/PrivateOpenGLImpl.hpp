#pragma once

#if defined(FLEUR_PLATFORM_WIN)
#define NOMINMAX
#include <windows.h>  // GDI needed: GetDC / ChoosePixelFormat / SetPixelFormat / SwapBuffers
#endif

#include <glad/gl.h>
#include <glad/wgl.h>

#include <glm/glm.hpp>
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
    uint32_t indexOffset;   // in indices
    int32_t vertexOffset;   // in vertices (baseVertex)
};

struct backend::impl
{
    impl(bool enableValidation, void* pNativeHandle, Fleur::SRect& framebufferSize, SFLImageView& fallback);
    ~impl();

    // ---------- context (mirrors FVkDevice) ----------
    HWND m_Hwnd{nullptr};
    HDC m_Hdc{nullptr};
    HGLRC m_Ctx{nullptr};
    void createContext(void* pNativeHandle);

    int m_Width{0};
    int m_Height{0};

    // ---------- frame ----------
    glm::mat4 m_View{1.0f};
    glm::mat4 m_Proj{1.0f};
    glm::vec3 m_CameraDir{0.0f};
    void beginFrame(SFLCameraData& cameraData);
    void draw(AssetID model, const glm::mat4& transform);
    void endFrame();

    // ---------- resources ----------
    std::unordered_map<AssetID, std::vector<GlPrimitive>> m_RegisteredModels;
    void registerModel(AssetID id, const SVertexData* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount,
                       const FLDrawItem* primitives, uint32_t primitiveCount);
    void unregisterModel(AssetID id);

    std::unordered_map<AssetID, GLuint> m_TextureMap;
    AssetID m_FallbackTexture{0};
    void uploadTextures(SFLImageViewInfo* pInfo);
    void removeTexture(AssetID id);

    // ---------- passes / skybox (filled in later stages) ----------
    void createPass(EFLPassKind kind, SFLShaderStages shaderStages);
    void createSkybox(AssetID id, SFLShaderStages shaderStages);
    void setSkybox(AssetID id);
};
}  // namespace gl
