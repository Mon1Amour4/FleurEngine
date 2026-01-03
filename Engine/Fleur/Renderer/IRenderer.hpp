#pragma once

#include "../WindowPrimitives.hpp"
#include "Graphics.hpp"
#include "glm/glm.hpp"

namespace Fleur::Graphics
{

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
    const char* shaderCode = nullptr;
    uint32_t sizeBytes = 0;
};

struct SFLGeometryPass
{
    SFLShaderInfo* pVertexShaderInfo = nullptr;
    SFLShaderInfo* pFragmentShaderInfo = nullptr;
    EFLVertexInputDescription vertexInputInfo = VERTEX_INPUT_MAX_VALUE;
    EFLIndexInputDescription indexInputInfo = INDEX_INPUT_MAX_VALUE;
    EFLInputAssemblyTopology inputAssemblyTopology = FL_INPUT_ASSEMBLY_TOPOLOGY_TRIANGLE_MAX_VALUE;
};

struct SFLFrame
{
    SFLGeometryPass* pPass = nullptr;
};

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
    virtual void Update(Fleur::Graphics::SFLGeometryUBO* pUbo) = 0;
    virtual void ResizeEvent(Fleur::SRect& rect) = 0;
};

}  // namespace Fleur::Graphics
