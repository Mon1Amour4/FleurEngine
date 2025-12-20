#pragma once
#include "Graphics.hpp"

namespace Fleur::Graphics
{
class Surface
{
public:
    virtual ~Surface() = default;
    virtual Rect GetRect() const = 0;

    virtual const void* GetNativeHandle() const = 0;

    virtual void Release() = 0;
};
}  // namespace Fleur::Graphics
