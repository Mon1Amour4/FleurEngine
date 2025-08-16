#pragma once

#include <QuartzCore/CAMetalLayer.h>
#include <simd/simd.h>

#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>
#include <QuartzCore/QuartzCore.hpp>

namespace Fleur
{
class MetalContext : public GraphicsContext
{
public:
    MetalContext(void* nsWindow);
    virtual ~MetalContext() override;

    virtual bool Init() override;
    virtual void SwapBuffers() override;

    // Temporary code to draw a triangle as fast as possible
    MTL::Buffer* triangleVertexBuffer;
    MTL::CommandQueue* metalCommandQueue;
    MTL::RenderPipelineState* metalRenderPSO;

private:
    void* _window;
    MTL::Device* _device;
    CA::MetalDrawable* _metalDrawable;
    void* _metalLayer;
};
}  // namespace Fleur
