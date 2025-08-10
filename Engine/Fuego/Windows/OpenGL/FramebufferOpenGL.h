#pragma once

#include "Framebuffer.h"

namespace Fuego::Graphics
{

class FramebufferOpenGL : public Framebuffer
{
public:
    FramebufferOpenGL(uint32_t width, uint32_t height);
    FramebufferOpenGL(std::string_view name, uint32_t width, uint32_t height, uint32_t flags);

    virtual ~FramebufferOpenGL() override;

    virtual void Bind() override;
    virtual void Unbind() override;

    // virtual Fuego::Graphics::Texture* GetColorAttachment(int index = 0) override;
    // virtual Fuego::Graphics::Texture* GetDepthAttachment() override;

    virtual void ResizeFBO(uint32_t width, uint32_t height) override;

    virtual uint32_t ID() const
    {
        return fbo;
    }

    virtual void Release() override;

    virtual void Clear() override;

protected:
    uint32_t fbo;

private:
    virtual void Init() override;
    virtual void Cleanup() override;

    uint32_t color, depth, stencil;
    std::string name;
};

class DefaultFramebufferOpenGL : public FramebufferOpenGL
{
public:
    DefaultFramebufferOpenGL(uint32_t width, uint32_t height);
    ~DefaultFramebufferOpenGL() = default;

    virtual void Bind() override;
    virtual void Unbind() override;

    virtual void ResizeFBO(uint32_t width, uint32_t height) override;

    virtual void Release() override;

    virtual void Clear() override;
};
}  // namespace Fuego::Graphics