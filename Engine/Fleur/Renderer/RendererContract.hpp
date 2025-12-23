#pragma once

struct DrawInfo
{
    char* byte;
    size_t size;
};

struct IRenderer
{
    virtual void Draw(DrawInfo info) = 0;
};