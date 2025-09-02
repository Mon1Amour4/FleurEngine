#pragma once

#include "FramebufferOpenGL.h"
#include "Renderer/CommandBuffer.h"

namespace Fleur::Graphics
{
class Surface;

struct VertexLayout;

class CommandBufferOpenGL final : public CommandBuffer
{
    friend class DeviceOpenGL;

public:
    virtual ~CommandBufferOpenGL() override;
    virtual void BeginRecording() override;
    virtual void EndRecording() override;
    virtual void Submit() override;
    virtual void BindRenderTarget(const Framebuffer& fbo, EFramebufferRWOperation rw) override;
    virtual void BindShaderObject(std::shared_ptr<Fleur::Graphics::ShaderObject> shader) override;
    virtual void BindDescriptorSet(const DescriptorBuffer& descriptorSet, int setIndex) override;

    virtual void BindVertexBuffer(std::unique_ptr<Buffer> vertexBuffer, VertexLayout layout) override;
    virtual void BindIndexBuffer(std::unique_ptr<Buffer> buffer) override;

    virtual size_t UpdateBufferSubDataImpl(Buffer::EBufferType type, const void* data, size_t sizeBytes) override;

    virtual void BindTexture(Texture* texture) override;
    virtual void Draw(uint32_t vertexCount) override;
    virtual void IndexedDraw(uint32_t indexCount, size_t indexOffsetBytes, uint32_t baseVertex) override;

    virtual void PushDebugGroup(uint32_t id, const char* message) override;
    virtual void PopDebugGroup() override;
    virtual void SetLabel(EObjectLabel id, uint32_t name, const char* message) override;

private:
    int ConvertUsage(ERenderStage& stage) const;
    uint32_t m_VAO, m_Texture;
    int m_MainVsShader, m_PixelShader;
    bool m_IsLinked, m_IsDataAllocated, m_IsFree;

    CommandBufferOpenGL(DepthStencilDescriptor desc);

    uint32_t GetDeathFuncOp(EDepthTestOperation op) const;
};
}  // namespace Fleur::Graphics
