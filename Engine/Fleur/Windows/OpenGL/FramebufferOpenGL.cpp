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
    , name("default")
    , fbo(0)
{
}

Fleur::Graphics::FramebufferOpenGL::FramebufferOpenGL(std::string_view name, uint32_t width, uint32_t height, uint32_t flags)
    : Framebuffer(width, height, flags)
    , name(name)
    , fbo(0)
{
    glCreateFramebuffers(1, &fbo);
    glObjectLabel(GL_FRAMEBUFFER, fbo, -1, this->name.c_str());
}

void Fleur::Graphics::FramebufferOpenGL::Clear()
{
    Bind();

    uint32_t gl_flags = 0;
    if (flags & static_cast<uint32_t>(FramebufferSettings::COLOR))
        gl_flags |= GL_COLOR_BUFFER_BIT;

    if (flags & static_cast<uint32_t>(FramebufferSettings::DEPTH_STENCIL))
    {
        gl_flags |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    }
    else
    {
        if (flags & static_cast<uint32_t>(FramebufferSettings::DEPTH))
            gl_flags |= GL_DEPTH_BUFFER_BIT;
        if (flags & static_cast<uint32_t>(FramebufferSettings::STENCIL))
            gl_flags |= GL_STENCIL_BUFFER_BIT;
    }

    glClear(gl_flags);
}

void Fleur::Graphics::FramebufferOpenGL::AddColorAttachment(std::shared_ptr<Fleur::Graphics::Texture> attachment)
{
    Framebuffer::AddColorAttachment(attachment);

    Fleur::Graphics::TextureOpenGL* texture_gl = static_cast<Fleur::Graphics::TextureOpenGL*>(color_attachments.back().get());

    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, *texture_gl->GetTextureID(), 0);

    if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        FL_CORE_ASSERT(false, "");
}

void Fleur::Graphics::FramebufferOpenGL::AddDepthAttachment(std::shared_ptr<Fleur::Graphics::Texture> attachment, bool combined)
{
    Framebuffer::AddDepthAttachment(attachment);

    Fleur::Graphics::TextureOpenGL* texture_gl = static_cast<Fleur::Graphics::TextureOpenGL*>(depth_attachment.get());

    if (combined)
        glNamedFramebufferTexture(fbo, GL_DEPTH_STENCIL_ATTACHMENT, *texture_gl->GetTextureID(), 0);
    else
        glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, *texture_gl->GetTextureID(), 0);

    if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        FL_CORE_ASSERT(false, "");
}

void Fleur::Graphics::FramebufferOpenGL::AddStencilAttachment(std::shared_ptr<Fleur::Graphics::Texture> attachment)
{
    Framebuffer::AddStencilAttachment(attachment);

    Fleur::Graphics::TextureOpenGL* texture_gl = static_cast<Fleur::Graphics::TextureOpenGL*>(stencil_attachment.get());

    glNamedFramebufferTexture(fbo, GL_STENCIL_ATTACHMENT, *texture_gl->GetTextureID(), 0);

    if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        FL_CORE_ASSERT(false, "");
}

Fleur::Graphics::FramebufferOpenGL::~FramebufferOpenGL()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Cleanup();
    if (fbo)
        glDeleteFramebuffers(1, &fbo);
}

void Fleur::Graphics::FramebufferOpenGL::Cleanup()
{
    if (color_attachments.size() > 0)
    {
        for (auto& attach : color_attachments)
        {
            TextureOpenGL* texture_gl = static_cast<TextureOpenGL*>(attach.get());
            glDeleteTextures(1, texture_gl->GetTextureID());
        }
    }

    if (depth_attachment)
    {
        TextureOpenGL* texture_gl = static_cast<TextureOpenGL*>(depth_attachment.get());
        glDeleteTextures(1, texture_gl->GetTextureID());
    }

    if (stencil_attachment)
    {
        TextureOpenGL* texture_gl = static_cast<TextureOpenGL*>(stencil_attachment.get());
        glDeleteTextures(1, texture_gl->GetTextureID());
    }
}

void Fleur::Graphics::FramebufferOpenGL::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);
}

void Fleur::Graphics::FramebufferOpenGL::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
