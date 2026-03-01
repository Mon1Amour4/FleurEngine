#pragma once

#include "../WindowPrimitives.hpp"
#include "Graphics.hpp"
#include "RenderViews.hpp"
#include "glm/glm.hpp"

namespace Fleur::Graphics
{

#pragma region Structs&Enums

enum EFLInputAssemblyTopology
{
    FL_INPUT_ASSEMBLY_TOPOLOGY_TRIANGLE_LIST,
    FL_INPUT_ASSEMBLY_TOPOLOGY_TRIANGLE_STRIP,
    FL_INPUT_ASSEMBLY_TOPOLOGY_TRIANGLE_FAN,
    FL_INPUT_ASSEMBLY_TOPOLOGY_TRIANGLE_MAX_VALUE,
};
enum EFLVertexInputDescription
{
    VERTEX_INPUT_VERTEX_DATA,
    VERTEX_INPUT_MAX_VALUE
};
enum EFLIndexInputDescription
{
    INDEX_INPUT_UINT16,
    INDEX_INPUT_UINT32,
    INDEX_INPUT_MAX_VALUE
};

struct SFLShaderInfo
{
    const char* shaderCode;
    uint32_t sizeBytes;

    SFLShaderInfo()
        : shaderCode(nullptr)
        , sizeBytes(0) {};
};

struct SFLGeometryPass
{
    SFLShaderInfo* pVertexShaderInfo;
    SFLShaderInfo* pFragmentShaderInfo;
    EFLVertexInputDescription vertexInputInfo;
    EFLIndexInputDescription indexInputInfo;
    EFLInputAssemblyTopology inputAssemblyTopology;

    SFLGeometryPass()
        : pVertexShaderInfo(nullptr)
        , pFragmentShaderInfo(nullptr)
        , vertexInputInfo(VERTEX_INPUT_MAX_VALUE)
        , indexInputInfo(INDEX_INPUT_MAX_VALUE)
        , inputAssemblyTopology(FL_INPUT_ASSEMBLY_TOPOLOGY_TRIANGLE_MAX_VALUE) {};
};

struct SFLFrame
{
    SFLGeometryPass* pPass;

    SFLFrame()
        : pPass(nullptr) {};
};

struct SFLDrawUploadInfo
{
    const void* pVertex;
    uint64_t vertexCount;
    const void* pIndex;
    uint64_t indexCount;

    SFLDrawUploadInfo()
        : pVertex(nullptr)
        , vertexCount(0)
        , pIndex(nullptr)
        , indexCount(0) {};
};

#pragma endregion

struct IRenderer
{
    virtual ~IRenderer() = default;

    virtual void AddToDrawList(Fleur::Graphics::SFLModelView* pModelView) = 0;
    virtual void Update(Fleur::Graphics::SFLCameraData& cameraData) = 0;
    virtual void SubmitImageViews(Fleur::Graphics::SFLImageViewInfo* pInfo) = 0;

    virtual void StartResize() = 0;
    virtual void EndResize(Fleur::SRect& rect) = 0;

    virtual void CreateSkybox(AssetID id, SFLShaderInfo* pVertexShaderInfo, SFLShaderInfo* pFragmentShaderInfo) = 0;
    virtual void SetSkybox(AssetID id) = 0;
};

}  // namespace Fleur::Graphics
