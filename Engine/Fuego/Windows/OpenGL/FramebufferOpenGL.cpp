#include "FramebufferOpenGL.h"

#include "glad/gl.h"

Fuego::Graphics::DefaultFramebufferOpenGL::DefaultFramebufferOpenGL(uint32_t width, uint32_t height)
    : FramebufferOpenGL(width, height)
{
}

void Fuego::Graphics::DefaultFramebufferOpenGL::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void Fuego::Graphics::DefaultFramebufferOpenGL::Unbind()
{
}

void Fuego::Graphics::DefaultFramebufferOpenGL::ResizeFBO(uint32_t width, uint32_t height)
{
    this->width = width;
    this->height = height;
}

void Fuego::Graphics::DefaultFramebufferOpenGL::Release()
{
}

void Fuego::Graphics::DefaultFramebufferOpenGL::Clear()
{
    Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

Fuego::Graphics::FramebufferOpenGL::FramebufferOpenGL(uint32_t width, uint32_t height)
    : Framebuffer(width, height, 0)
    , color(0)
    , depth(0)
    , stencil(0)
    , name("default")
    , fbo(0)
{
}

Fuego::Graphics::FramebufferOpenGL::FramebufferOpenGL(std::string_view name, uint32_t width, uint32_t height, uint32_t flags)
    : Framebuffer(width, height, flags)
    , color(0)
    , depth(0)
    , stencil(0)
    , name(name)
    , fbo(0)
{
    glCreateFramebuffers(1, &fbo);
    Init();
    glObjectLabel(GL_FRAMEBUFFER, fbo, -1, this->name.c_str());
}

void Fuego::Graphics::FramebufferOpenGL::Release()
{
    Cleanup();

    if (fbo)
        glDeleteFramebuffers(1, &fbo);
}

void Fuego::Graphics::FramebufferOpenGL::Clear()
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

void Fuego::Graphics::FramebufferOpenGL::Init()
{
    if (flags & static_cast<uint32_t>(FramebufferSettings::COLOR))
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &color);

        glTextureStorage2D(color, 1, GL_RGBA8, width, height);

        glTextureParameteri(color, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(color, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, color, 0);
    }

    if ((flags & static_cast<uint32_t>(FramebufferSettings::DEPTH)) && !(flags & static_cast<uint32_t>(FramebufferSettings::DEPTH_STENCIL)))
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &depth);
        glTextureStorage2D(depth, 1, GL_DEPTH_COMPONENT32F, width, height);

        glTextureParameteri(depth, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(depth, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, depth, 0);
    }

    if ((flags & static_cast<uint32_t>(FramebufferSettings::STENCIL)) && !(flags & static_cast<uint32_t>(FramebufferSettings::DEPTH_STENCIL)))
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &stencil);
        glTextureStorage2D(stencil, 1, GL_STENCIL_INDEX8, width, height);

        glTextureParameteri(stencil, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(stencil, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glNamedFramebufferTexture(fbo, GL_STENCIL_ATTACHMENT, stencil, 0);
    }

    if (flags & static_cast<uint32_t>(FramebufferSettings::DEPTH_STENCIL))
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &depth);
        glTextureStorage2D(depth, 1, GL_DEPTH24_STENCIL8, width, height);

        glTextureParameteri(depth, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(depth, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glNamedFramebufferTexture(fbo, GL_DEPTH_STENCIL_ATTACHMENT, depth, 0);
    }

    if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        FU_CORE_ASSERT(false, "");
}

Fuego::Graphics::FramebufferOpenGL::~FramebufferOpenGL()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Cleanup();
}

void Fuego::Graphics::FramebufferOpenGL::Cleanup()
{
    if (color)
        glDeleteTextures(1, &color);

    if (depth)
        glDeleteTextures(1, &depth);

    if (stencil)
        glDeleteTextures(1, &stencil);
}

void Fuego::Graphics::FramebufferOpenGL::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void Fuego::Graphics::FramebufferOpenGL::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Fuego::Graphics::Texture* Fuego::Graphics::FramebufferOpenGL::GetColorAttachment(int index)
//{
//     return nullptr;
// }
//
// Fuego::Graphics::Texture* Fuego::Graphics::FramebufferOpenGL::GetDepthAttachment()
//{
//     return nullptr;
// }

void Fuego::Graphics::FramebufferOpenGL::ResizeFBO(uint32_t width, uint32_t height)
{
    this->width = width;
    this->height = height;

    Cleanup();
    Init();
}
