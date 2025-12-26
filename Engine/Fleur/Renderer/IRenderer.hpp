#pragma once

#include "../WindowPrimitives.hpp"

struct DrawInfo
{
    char* byte;
    size_t size;
};

struct IRenderer
{
    virtual ~IRenderer() = default;
    virtual void Draw(DrawInfo info) = 0;
};