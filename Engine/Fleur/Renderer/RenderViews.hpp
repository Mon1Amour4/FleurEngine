#pragma once

namespace Fleur::Graphics
{

struct SFLImageView
{
    uint32_t ID;
    const char* pData = nullptr;
    uint32_t w = 0;
    uint32_t h = 0;
    uint32_t layerCount = 0;
    uint32_t channels;
};
struct SFLImageViewInfo
{
    SFLImageView* pData;
    uint32_t count;
};

struct SFLMaterialView
{
    uint32_t albedoID;
    uint32_t normalID;
};
struct SFLMaterialViewInfo
{
    SFLMaterialView* pData;
    uint32_t count;
};

struct SFLMeshView
{
    uint64_t indexCount;
    uint64_t vertexCount;

    uint32_t materialIdx;
};
struct SFLMeshViewInfo
{
    SFLMeshView* pData;
    uint32_t count;
};

struct SFLBufferView
{
    const void* pData;
    uint64_t count;
};

struct SFLModelView
{
    SFLBufferView vertecies;
    SFLBufferView indecies;

    SFLMeshViewInfo meshes;

    SFLMaterialViewInfo materials;
};

}  // namespace Fleur::Graphics
