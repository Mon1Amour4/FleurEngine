#pragma once

#include "Graphics.hpp"

namespace Fleur::Graphics
{
struct SFLImageView
{
    AssetID ID;
    const char* pData{};
    uint32_t w{};
    uint32_t h{};
    uint32_t layerCount{};
    uint32_t channels{};
    bool srgb{true};
};
struct SFLImageViewInfo
{
    SFLImageView* pData{};
    uint32_t count{};
};

struct FLPrimitiveDrawItem
{
    uint32_t indexStart;
    uint32_t indexCount;
    uint32_t vertexStart;

    Fleur::Graphics::FLMaterial material;

    Fleur::Vec3 boundingBoxCenter;
};

struct FLInstanceItem
{
    uint32_t drawCount;
    uint32_t nodeTransformStartIdx;

    uint32_t primitiveCount;
};

// Aggregates everything needed to register one model with the renderer.
// Pointer+count pairs (not owning) — caller keeps data alive for the call.
struct SFLModelRegisterInfo
{
    AssetID model{};
    BoundingBox modelBoundingBox{};

    const SVertexData* vertices{};
    uint32_t vertexCount{};

    const uint32_t* indices{};
    uint32_t indexCount{};

    const Fleur::Mat4* nodeTransforms{};
    uint32_t nodeTransformCount{};

    const FLPrimitiveDrawItem* primitives{};
    uint32_t primitiveCount{};

    const FLInstanceItem* instances{};
    uint32_t instanceCount{};
};

}  // namespace Fleur::Graphics
