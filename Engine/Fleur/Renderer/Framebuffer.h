#pragma once

#include "Texture.h"

namespace Fleur::Graphics
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

enum class FramebufferRWOperation
{
    READ_ONLY,
    WRITE_ONLY,
    READ_WRITE
};

class Framebuffer
{
protected:
    Framebuffer(int width, int height, uint32_t flags)
        : flags(flags)
        , width(width)
        , height(height) {};

public:
    virtual ~Framebuffer() = default;

    virtual void Bind() = 0;
    virtual void Unbind() = 0;

    const Fleur::Graphics::Texture* GetColorAttachment(uint32_t index = 0) const
    {
        if (color_attachments.size() > 0 && index <= color_attachments.size() - 1)
            return color_attachments[index].get();
        return nullptr;
    }

    Fleur::Graphics::Texture* GetDepthAttachment() const
    {
        return depth_attachment.get();
    }

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

    virtual void Clear() = 0;

protected:
    virtual void Cleanup() = 0;

    uint32_t width, height, flags;

    std::vector<std::shared_ptr<Fleur::Graphics::Texture>> color_attachments;
    std::shared_ptr<Fleur::Graphics::Texture> depth_attachment;
    std::shared_ptr<Fleur::Graphics::Texture> stencil_attachment;

    virtual void AddColorAttachment(std::shared_ptr<Fleur::Graphics::Texture> attachment)
    {
        color_attachments.push_back(attachment);
    }
    virtual void AddDepthAttachment(std::shared_ptr<Fleur::Graphics::Texture> attachment, bool combined = true)
    {
        depth_attachment = attachment;
    }
    virtual void AddStencilAttachment(std::shared_ptr<Fleur::Graphics::Texture> attachment)
    {
        stencil_attachment = attachment;
    }
};
}  // namespace Fleur::Graphics