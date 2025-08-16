#include "SurfaceMetal.h"

#include <QuartzCore/CAMetalLayer.hpp>

namespace Fleur::Renderer
{
SurfaceMetal::SurfaceMetal(const void* window, MTL::Device* device)
{
    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    metalLayer.device = (__bridge id<MTLDevice>)device;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;

    _metalLayer = (__bridge_retained void*)metalLayer;

    NSWindow* w = (__bridge NSWindow*)window;
    w.contentView.layer = metalLayer;
    w.contentView.wantsLayer = YES;
}

// TODO: make API-agnostic
MTL::PixelFormat SurfaceMetal::GetPixelFormat() const
{
    return (MTL::PixelFormat)((__bridge CAMetalLayer*)_metalLayer).pixelFormat;
}

SurfaceMetal::~SurfaceMetal()
{
    __unused id metalLayerObj = (__bridge_transfer id)_metalLayer;
}

const void* SurfaceMetal::GetNativeHandle() const
{
    return _metalLayer;
}

}  // namespace Fleur::Renderer
