#pragma once

namespace Fuego::Graphics
{
class Surface;
class Framebuffer;

class Swapchain
{
public:
    virtual ~Swapchain() = default;

    virtual void Present() = 0;

    virtual void ShowWireFrame(bool show) = 0;
    virtual void ValidateWindow() = 0;

    virtual const Framebuffer& GetScreenTexture() = 0;

    virtual void ClearBackbuffer() const = 0;

    virtual void Release() = 0;
};
}  // namespace Fuego::Graphics
