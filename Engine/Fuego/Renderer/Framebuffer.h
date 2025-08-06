#pragma once

#include "Texture.h"

namespace Feugo::Fraphics
{
class Framebuffer
{
public:
    Framebuffer(int width, int height);
    virtual ~Framebuffer();

    virtual void Bind() = 0;
    virtual void Unbind() = 0;

    void Resize(int newWidth, int newHeight);

    virtual Fuego::Graphics::Texture* GetColorAttachment(int index = 0) = 0;
    virtual Fuego::Graphics::Texture* GetDepthAttachment() = 0;

private:
    virtual void Init();
    virtual void Cleanup();

    int width, height;

    std::vector<std::shared_ptr<Fuego::Graphics::Texture>> colorAttachments;
    std::shared_ptr<Fuego::Graphics::Texture> depthAttachment;
};
}  // namespace Feugo::Fraphics