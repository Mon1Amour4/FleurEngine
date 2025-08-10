#pragma once

#include "Texture.h"

namespace Fuego::Graphics
{
enum class FramebufferSettings : uint32_t
{
    COLOR = 1 << 0,
    DEPTH = 1 << 1,
    STENCIL = 1 << 2,
    DEPTH_STENCIL = 1 << 3
};

inline FramebufferSettings operator|(FramebufferSettings a, FramebufferSettings b)
{
    return static_cast<FramebufferSettings>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

class Framebuffer
{
public:
    Framebuffer(int width, int height, uint32_t flags)
        : flags(flags)
        , width(width)
        , height(height) {};

    virtual ~Framebuffer() = default;

    virtual void Bind() = 0;
    virtual void Unbind() = 0;

    // virtual Fuego::Graphics::Texture* GetColorAttachment(int index = 0) = 0;
    // virtual Fuego::Graphics::Texture* GetDepthAttachment() = 0;

    uint32_t Width() const
    {
        return width;
    }

    uint32_t Height() const
    {
        return height;
    }

    uint32_t Flags() const
    {
        return flags;
    }

    virtual void ResizeFBO(uint32_t width, uint32_t height) = 0;

    virtual void Release() = 0;

    virtual void Clear() = 0;

protected:
    virtual void Init() = 0;
    virtual void Cleanup() = 0;

    uint32_t width, height, flags;

    std::vector<std::shared_ptr<Fuego::Graphics::Texture>> colorAttachments;
    std::shared_ptr<Fuego::Graphics::Texture> depthAttachment;
};
}  // namespace Fuego::Graphics