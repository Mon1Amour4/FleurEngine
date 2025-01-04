#pragma once

#include <Metal/Metal.hpp>

#include "Renderer/CommandBuffer.h"

namespace Fuego::Renderer
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
    virtual void BindRenderTarget(const Surface& texture) override;
    virtual void BindVertexShader(const Shader& vertexShader) override;
    virtual void BindPixelShader(const Shader& pixelShader) override;
    virtual void BindDescriptorSet(const DescriptorBuffer& descriptorSet, int setIndex) override;
    virtual void BindVertexBuffer(const Buffer& vertexBuffer) override;
    virtual void BindIndexBuffer(uint32_t indices[], uint32_t size) override
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

private:
    MTL::CommandBuffer* _commandBuffer;
    MTL::Device* _device;

    const ShaderMetal* _vertexShader;
    const ShaderMetal* _pixelShader;
    const BufferMetal* _buffer;
    const SurfaceMetal* _surface;
};

}  // namespace Fuego::Renderer
