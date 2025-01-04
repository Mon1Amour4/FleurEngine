#pragma once

#include "Renderer/Swapchain.h"
#include "SurfaceMetal.h"

namespace Fuego::Renderer
{
class Surface;

class SwapchainMetal : public Swapchain
{
public:
    SwapchainMetal(const Surface& surface);
    ~SwapchainMetal();

    virtual void Present() override;

    virtual Surface& GetScreenTexture() override;
    virtual void ShowWireFrame(bool show) override
    {
        UNUSED(show);
    }
    virtual void ValidateWindow() override
    {
    }

private:
    SurfaceMetal _surface;
};
}  // namespace Fuego::Renderer
