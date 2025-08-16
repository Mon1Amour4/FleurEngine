#pragma once

#include <Metal/Metal.hpp>

#include "Renderer/CommandBuffer.h"

namespace Fleur::Renderer
{

class ShaderMetal;
class BufferMetal;
class SurfaceMetal;

class CommandBufferMetal : public CommandBuffer
{
public:
    CommandBufferMetal(MTL::CommandBuffer* commandBuffer, MTL::Device* device);
    ~CommandBufferMetal();

    virtual void BeginRecording() override
    {
    }
    virtual void EndRecording() override
    {
    }
    virtual void Submit() override
    {
    }

    virtual void BindShaderObject(const ShaderObject& obj) override
    { /* TODO */
    }
    virtual void BindRenderTarget(const Surface& texture) override;

    // TODO: remove
    virtual void BindVertexShader(const Shader& vertexShader);
    virtual void BindPixelShader(const Shader& pixelShader);

    virtual void BindDescriptorSet(const DescriptorBuffer& descriptorSet, int setIndex) override;
    virtual void BindVertexBuffer(const Buffer& vertexBuffer, VertexLayout layout) override;
    virtual void BindIndexBuffer(const uint32_t indices[], uint32_t size) override
    {
        UNUSED(indices);
        UNUSED(size);
    }
    virtual void Draw(uint32_t vertexCount) override;
    virtual void IndexedDraw(uint32_t vertexCount) override
    {
        UNUSED(vertexCount);
    }
    virtual void Clear() override
    {
    }

    virtual void PushDebugGroup(uint32_t id, const char* message) override;
    virtual void PopDebugGroup() override;
    virtual void SetLabel(ObjectLabel id, uint32_t name, const char* message) override;

private:
    MTL::CommandBuffer* _commandBuffer;
    MTL::Device* _device;

    const ShaderMetal* _vertexShader;
    const ShaderMetal* _pixelShader;
    const BufferMetal* _buffer;
    const SurfaceMetal* _surface;
};

}  // namespace Fleur::Renderer
