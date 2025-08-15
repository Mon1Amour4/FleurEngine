#pragma once

#include "Framebuffer.h"

namespace Fuego::Graphics
{

class DeviceOpenGL;

class FramebufferOpenGL : public Framebuffer
{
    friend class DeviceOpenGL;

public:
    FramebufferOpenGL(std::string_view name, uint32_t width, uint32_t height, uint32_t flags);

    virtual ~FramebufferOpenGL() override;

    virtual void Bind() override;
    virtual void Unbind() override;

    virtual uint32_t ID() const
    {
        return fbo;
    }

    virtual void Clear() override;

protected:
    uint32_t fbo;

    FramebufferOpenGL(uint32_t width, uint32_t height);
    virtual void AddColorAttachment(std::shared_ptr<Fuego::Graphics::Texture> attachment) override;
    virtual void AddDepthAttachment(std::shared_ptr<Fuego::Graphics::Texture> attachment, bool combined = true) override;
    virtual void AddStencilAttachment(std::shared_ptr<Fuego::Graphics::Texture> attachment) override;

private:
    virtual void Cleanup() override;

    std::string name;
};

class DefaultFramebufferOpenGL : public FramebufferOpenGL
{
    friend class DeviceOpenGL;

public:
    DefaultFramebufferOpenGL(uint32_t width, uint32_t height);
    ~DefaultFramebufferOpenGL() = default;

    virtual void Unbind() override;

    virtual void Clear() override;
};
}  // namespace Fuego::Graphics