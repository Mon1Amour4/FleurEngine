#pragma once

#include <Metal/Metal.hpp>

#include "Renderer/CommandBuffer.h"

namespace Fuego::Renderer
{

class CommandBufferMetal : public CommandBuffer
{
public:
    CommandBufferMetal(MTL::CommandBuffer* commandBuffer);
    ~CommandBufferMetal();

    virtual void BindRenderTarget(const Surface& texture) override;
    virtual void BindVertexShader(const Shader& vertexShader) override;
    virtual void BindPixelShader(const Shader& pixelShader) override;
    virtual void BindDescriptorSet(const DescriptorBuffer& descriptorSet, int setIndex) override;
    virtual void BindVertexBuffer(const Buffer& vertexBuffer) override;
    virtual void Draw(uint32_t vertexCount) override;

private:
    MTL::RenderPipelineDescriptor* _renderPipelineDescriptor;
    MTL::CommandBuffer* _commandBuffer;
};

}  // namespace Fuego::Renderer
