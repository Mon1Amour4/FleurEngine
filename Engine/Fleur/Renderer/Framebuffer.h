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
        : m_Flags(flags)
        , m_Width(width)
        , m_Height(height) {};

public:
    virtual ~Framebuffer() = default;

    virtual void Bind() = 0;
    virtual void Unbind() = 0;

    const Fleur::Graphics::Texture* GetColorAttachment(uint32_t index = 0) const
    {
        if (m_ColorAttachments.size() > 0 && index <= m_ColorAttachments.size() - 1)
            return m_ColorAttachments[index].get();
        return nullptr;
    }

    Fleur::Graphics::Texture* GetDepthAttachment() const
    {
        return m_DepthAttachment.get();
    }

    uint32_t Width() const
    {
        return m_Width;
    }

    uint32_t Height() const
    {
        return m_Height;
    }

    uint32_t Flags() const
    {
        return m_Flags;
    }

    virtual void Clear() = 0;

protected:
    virtual void Cleanup() = 0;

    uint32_t m_Width, m_Height, m_Flags;

    std::vector<std::shared_ptr<Fleur::Graphics::Texture>> m_ColorAttachments;
    std::shared_ptr<Fleur::Graphics::Texture> m_DepthAttachment;
    std::shared_ptr<Fleur::Graphics::Texture> m_StencilAttachment;

    virtual void AddColorAttachment(std::shared_ptr<Fleur::Graphics::Texture> attachment)
    {
        m_ColorAttachments.push_back(attachment);
    }
    virtual void AddDepthAttachment(std::shared_ptr<Fleur::Graphics::Texture> attachment, bool combined = true)
    {
        m_DepthAttachment = attachment;
    }
    virtual void AddStencilAttachment(std::shared_ptr<Fleur::Graphics::Texture> attachment)
    {
        m_StencilAttachment = attachment;
    }
};
}  // namespace Fleur::Graphics