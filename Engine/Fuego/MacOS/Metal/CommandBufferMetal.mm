#include "Metal/CommandBufferMetal.h"
#include "Metal/BufferMetal.h"
#include "Metal/ShaderMetal.h"
#include "Metal/SurfaceMetal.h"
#include "Metal/TextureMetal.h"

#include <QuartzCore/CAMetalLayer.hpp>

namespace Fuego::Renderer
{

CommandBufferMetal::CommandBufferMetal(MTL::CommandBuffer* commandBuffer, MTL::Device* device)
{
    _commandBuffer = commandBuffer;
    _device = device;
    _renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    _renderPipelineDescriptor->setLabel(NS::String::string("Default Rendering Pipeline", NS::ASCIIStringEncoding));
    MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
}

CommandBufferMetal::~CommandBufferMetal()
{
    _renderPipelineDescriptor->release();
    _renderCommandEncoder->release();
    _commandBuffer->release();
}

void CommandBufferMetal::BindRenderTarget(const Surface& texture)
{
    const auto* metalSurface = static_cast<const SurfaceMetal*>(&texture);
    MTL::PixelFormat pixelFormat = metalSurface->GetPixelFormat();
    _renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(pixelFormat);
    FU_CORE_ASSERT(_renderPipelineDescriptor, "Failed to bind Render Target");

    CAMetalLayer* layer = (__bridge CAMetalLayer*)metalSurface->GetNativeHandle();
    _metalDrawable = (__bridge CA::MetalDrawable*)[layer nextDrawable];
    FU_CORE_ASSERT(_metalDrawable, "Failed to get next drawable from CAMetalLayer");

    MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    MTL::RenderPassColorAttachmentDescriptor* cd = renderPassDescriptor->colorAttachments()->object(0);
    cd->setTexture(_metalDrawable->texture());
    cd->setLoadAction(MTL::LoadActionClear);
    cd->setClearColor(MTL::ClearColor(41.0f / 255.0f, 42.0f / 255.0f, 48.0f / 255.0f, 1.0));
    cd->setStoreAction(MTL::StoreActionStore);

    _renderCommandEncoder = _commandBuffer->renderCommandEncoder(renderPassDescriptor);
}

void CommandBufferMetal::BindVertexShader(const Shader& vertexShader)
{
    const auto* metalShader = static_cast<const ShaderMetal*>(&vertexShader);
    _renderPipelineDescriptor->setVertexFunction(metalShader->_function);
    FU_CORE_ASSERT(_renderPipelineDescriptor, "Failed to bind Vertex Shader");
}

void CommandBufferMetal::BindPixelShader(const Shader& pixelShader)
{
    const auto* metalShader = static_cast<const ShaderMetal*>(&pixelShader);
    _renderPipelineDescriptor->setFragmentFunction(metalShader->_function);
    FU_CORE_ASSERT(_renderPipelineDescriptor, "Failed to bind Pixel Shader");
}

void CommandBufferMetal::BindDescriptorSet(const DescriptorBuffer& descriptorSet, int setIndex)
{
}

void CommandBufferMetal::BindVertexBuffer(const Buffer& vertexBuffer)
{
    const auto* metalVertexBuffer = static_cast<const BufferMetal*>(&vertexBuffer);

    NS::Error* error;
    auto metalRenderPSO = _device->newRenderPipelineState(_renderPipelineDescriptor, &error);

    _renderCommandEncoder->setRenderPipelineState(metalRenderPSO);
    _renderCommandEncoder->setVertexBuffer(metalVertexBuffer->_buffer, 0, 0);
}

void CommandBufferMetal::Draw(uint32_t vertexCount)
{
    MTL::PrimitiveType typeTriangle = MTL::PrimitiveTypeTriangle;
    NS::UInteger vertexStart = 0;
    _renderCommandEncoder->drawPrimitives(typeTriangle, vertexStart, vertexCount);

    _renderCommandEncoder->endEncoding();

    _commandBuffer->presentDrawable(_metalDrawable);
    _commandBuffer->commit();
    _commandBuffer->waitUntilCompleted();
}

}  // namespace Fuego::Renderer
