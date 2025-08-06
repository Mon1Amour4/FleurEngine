#pragma once

#include "Texture.h"

namespace Fuego::Graphics
{
class Framebuffer
{
public:
    Framebuffer(int width, int height)
        : width(width)
        , height(height) {};
    virtual ~Framebuffer() = default;

    virtual void Bind() = 0;
    virtual void Unbind() = 0;

    virtual Fuego::Graphics::Texture* GetColorAttachment(int index = 0) = 0;
    virtual Fuego::Graphics::Texture* GetDepthAttachment() = 0;

private:
    virtual void Init() = 0;
    virtual void Cleanup() = 0;

    int width, height;

    std::vector<std::shared_ptr<Fuego::Graphics::Texture>> colorAttachments;
    std::shared_ptr<Fuego::Graphics::Texture> depthAttachment;
};
}  // namespace Fuego::Graphics