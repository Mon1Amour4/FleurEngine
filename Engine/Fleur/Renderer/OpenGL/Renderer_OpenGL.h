#pragma once

#include <windows.h>

#include <iostream>

#include "../IRenderer.hpp"

namespace Renderer::Backend
{
class RendererOpenGL : public IRenderer
{
    virtual void Draw(DrawInfo info) override;
};
}  // namespace Renderer::Backend