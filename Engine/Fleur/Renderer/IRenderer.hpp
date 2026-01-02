#pragma once

#include "../WindowPrimitives.hpp"

struct SFLDrawUploadInfo
{
    const void* pVertex = nullptr;
    uint64_t vertexCount = 0;
    const void* pIndex = nullptr;
    uint64_t indexCount;
};

struct IRenderer
{
    virtual ~IRenderer() = default;

    virtual void AddToDrawList(SFLDrawUploadInfo* pInfo) = 0;
    virtual void Update(float dtTime) = 0;
    virtual void ResizeEvent(Fleur::SRect& rect) = 0;
};