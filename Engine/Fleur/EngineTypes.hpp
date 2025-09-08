#pragma once

namespace Fleur::Engine
{

struct FLEUR_API IRenderer
{
    virtual ~IRenderer() = default;

    virtual void DrawModel() = 0;
    virtual void Clear() = 0;
    virtual void SwapBuffers() = 0;
};

}  // namespace Fleur::Engine