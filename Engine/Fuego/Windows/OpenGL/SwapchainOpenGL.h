#pragma once

#include "FramebufferOpenGL.h"
#include "Renderer/Swapchain.h"
#include "SurfaceOpenGL.h"

namespace Fuego::Graphics
{

class Surface;

class SwapchainOpenGL final : public Swapchain
{
public:
    FUEGO_NON_COPYABLE_NON_MOVABLE(SwapchainOpenGL);

    virtual ~SwapchainOpenGL() override;

    virtual void Present() override;

    virtual const Framebuffer& GetScreenTexture() override;

    virtual void ShowWireFrame(bool show) override;

    virtual void ValidateWindow() override;

    virtual void ClearBackbuffer() const override;

    virtual void Release() override;

private:
    std::unique_ptr<SurfaceOpenGL> _surface;
    std::unique_ptr<DefaultFramebufferOpenGL> _backbuffer;

protected:
    friend class DeviceOpenGL;
    SwapchainOpenGL(std::unique_ptr<Surface> _surface);
};
}  // namespace Fuego::Graphics
