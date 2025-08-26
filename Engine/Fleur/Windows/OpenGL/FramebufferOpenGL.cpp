#include "FramebufferOpenGL.h"

#include "TextureOpenGL.h"
#include "glad/gl.h"

Fleur::Graphics::DefaultFramebufferOpenGL::DefaultFramebufferOpenGL(uint32_t width, uint32_t height)
    : FramebufferOpenGL(width, height)
{
}

void Fleur::Graphics::DefaultFramebufferOpenGL::Unbind()
{
}

void Fleur::Graphics::DefaultFramebufferOpenGL::Clear()
{
    Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

Fleur::Graphics::FramebufferOpenGL::FramebufferOpenGL(uint32_t width, uint32_t height)
    : Framebuffer(width, height, 0)
    , m_Name("default")
    , m_FBO(0)
{
}

Fleur::Graphics::FramebufferOpenGL::FramebufferOpenGL(std::string_view name, uint32_t width, uint32_t height, uint32_t flags)
    : Framebuffer(width, height, flags)
    , m_Name(name)
    , m_FBO(0)
{
    glCreateFramebuffers(1, &m_FBO);
    glObjectLabel(GL_FRAMEBUFFER, m_FBO, -1, this->m_Name.c_str());
}

void Fleur::Graphics::FramebufferOpenGL::Clear()
{
    Bind();

    uint32_t flagsGL = 0;
    if (m_Flags & static_cast<uint32_t>(EFramebufferSettings::COLOR))
        flagsGL |= GL_COLOR_BUFFER_BIT;

    if (m_Flags & static_cast<uint32_t>(EFramebufferSettings::DEPTH_STENCIL))
    {
        flagsGL |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    }
    else
    {
        if (m_Flags & static_cast<uint32_t>(EFramebufferSettings::DEPTH))
            flagsGL |= GL_DEPTH_BUFFER_BIT;
        if (m_Flags & static_cast<uint32_t>(EFramebufferSettings::STENCIL))
            flagsGL |= GL_STENCIL_BUFFER_BIT;
    }

    glClear(flagsGL);
}

void Fleur::Graphics::FramebufferOpenGL::AddColorAttachment(std::shared_ptr<Fleur::Graphics::Texture> attachment)
{
    Framebuffer::AddColorAttachment(attachment);

    Fleur::Graphics::TextureOpenGL* textureGL = static_cast<Fleur::Graphics::TextureOpenGL*>(m_ColorAttachments.back().get());

    glNamedFramebufferTexture(m_FBO, GL_COLOR_ATTACHMENT0, *textureGL->GetTextureID(), 0);

    if (glCheckNamedFramebufferStatus(m_FBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        FL_CORE_ASSERT(false, "");
}

void Fleur::Graphics::FramebufferOpenGL::AddDepthAttachment(std::shared_ptr<Fleur::Graphics::Texture> attachment, bool combined)
{
    Framebuffer::AddDepthAttachment(attachment);

    Fleur::Graphics::TextureOpenGL* textureGL = static_cast<Fleur::Graphics::TextureOpenGL*>(m_DepthAttachment.get());

    if (combined)
        glNamedFramebufferTexture(m_FBO, GL_DEPTH_STENCIL_ATTACHMENT, *textureGL->GetTextureID(), 0);
    else
        glNamedFramebufferTexture(m_FBO, GL_DEPTH_ATTACHMENT, *textureGL->GetTextureID(), 0);

    if (glCheckNamedFramebufferStatus(m_FBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        FL_CORE_ASSERT(false, "");
}

void Fleur::Graphics::FramebufferOpenGL::AddStencilAttachment(std::shared_ptr<Fleur::Graphics::Texture> attachment)
{
    Framebuffer::AddStencilAttachment(attachment);

    Fleur::Graphics::TextureOpenGL* textureGL = static_cast<Fleur::Graphics::TextureOpenGL*>(m_StencilAttachment.get());

    glNamedFramebufferTexture(m_FBO, GL_STENCIL_ATTACHMENT, *textureGL->GetTextureID(), 0);

    if (glCheckNamedFramebufferStatus(m_FBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        FL_CORE_ASSERT(false, "");
}

Fleur::Graphics::FramebufferOpenGL::~FramebufferOpenGL()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Cleanup();
    if (m_FBO)
        glDeleteFramebuffers(1, &m_FBO);
}

void Fleur::Graphics::FramebufferOpenGL::Cleanup()
{
    if (m_ColorAttachments.size() > 0)
    {
        for (auto& attach : m_ColorAttachments)
        {
            TextureOpenGL* textureGL = static_cast<TextureOpenGL*>(attach.get());
            glDeleteTextures(1, textureGL->GetTextureID());
        }
    }

    if (m_DepthAttachment)
    {
        TextureOpenGL* textureGL = static_cast<TextureOpenGL*>(m_DepthAttachment.get());
        glDeleteTextures(1, textureGL->GetTextureID());
    }

    if (m_StencilAttachment)
    {
        TextureOpenGL* textureGL = static_cast<TextureOpenGL*>(m_StencilAttachment.get());
        glDeleteTextures(1, textureGL->GetTextureID());
    }
}

void Fleur::Graphics::FramebufferOpenGL::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glViewport(0, 0, m_Width, m_Height);
}

void Fleur::Graphics::FramebufferOpenGL::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
