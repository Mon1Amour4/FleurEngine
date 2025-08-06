#pragma once

#include "Framebuffer.h"

namespace Fuego::Graphics
{
class FramebufferOpenGL : public Framebuffer
{
public:
    FramebufferOpenGL(int width, int height);

    virtual ~FramebufferOpenGL() override;

    virtual void Bind() override;
    virtual void Unbind() override;

    virtual Fuego::Graphics::Texture* GetColorAttachment(int index = 0) override;
    virtual Fuego::Graphics::Texture* GetDepthAttachment() override;

private:
    virtual void Init() override;
    virtual void Cleanup() override;
};
}  // namespace Fuego::Graphics