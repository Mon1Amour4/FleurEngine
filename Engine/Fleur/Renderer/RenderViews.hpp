#pragma once

#include "Graphics.hpp"

namespace Fleur::Graphics
{
using AssetID = uint32_t;

struct SFLImageView
{
    AssetID ID;
    const char* pData{};
    uint32_t w{};
    uint32_t h{};
    uint32_t layerCount{};
    uint32_t channels{};
};
struct SFLImageViewInfo
{
    SFLImageView* pData{};
    uint32_t count{};
};

struct FLDrawItem
{
    uint32_t indexStart;
    uint32_t indexCount;
    uint32_t vertexStart;
    uint32_t albedoId;
    FLAlphaMode bucket{FL_OPAQUE};
};

}  // namespace Fleur::Graphics
