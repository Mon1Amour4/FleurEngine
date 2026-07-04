#include "Model.h"

#include <span>

#include "FleurAllocator.hpp"

//======================================================================
// Model
Fleur::Graphics::Model::Model(std::string_view modelName)
    : m_Name(modelName)
    , m_PrimitiveCount(0)
{
}

Fleur::Graphics::Model::Model(Model&& other) noexcept
    : m_Name(std::move(other.m_Name))
    , m_PrimitiveCount(other.m_PrimitiveCount)
    , m_Meshes(std::move(other.m_Meshes))
    , m_Vertices(std::move(other.m_Vertices))
    , m_Indices(std::move(other.m_Indices))
    , m_Materials(std::move(other.m_Materials))
    , m_MeshInstance(std::move(other.m_MeshInstance))
    , m_WorldTransforms(std::move(other.m_WorldTransforms))
{
    other.m_PrimitiveCount = 0;
}

Fleur::Graphics::Model& Fleur::Graphics::Model::operator=(Model&& other) noexcept
{
    if (this != &other)
    {
        m_Name = std::move(other.m_Name);

        m_PrimitiveCount = other.m_PrimitiveCount;

        m_Meshes = std::move(other.m_Meshes);
        m_Vertices = std::move(other.m_Vertices);
        m_Indices = std::move(other.m_Indices);
        m_Materials = std::move(other.m_Materials);
        m_MeshInstance = std::move(other.m_MeshInstance);
        m_WorldTransforms = std::move(other.m_WorldTransforms);

        other.m_PrimitiveCount = 0;
    }
    return *this;
}

//======================================================================
// Model::Mesh

Fleur::Graphics::Model::Mesh::Mesh(Mesh&& other) noexcept
    : m_Primitives(std::move(other.m_Primitives))
    , m_MeshName(std::move(other.m_MeshName))
    , m_MeshVertexStart(other.m_MeshVertexStart)
    , m_MeshVertexEnd(other.m_MeshVertexEnd)
    , m_MeshIndexStart(other.m_MeshIndexStart)
    , m_MeshIndexEnd(other.m_MeshIndexEnd)
    , m_MeshVertexCount(other.m_MeshVertexCount)
    , m_MeshIndicesCount(other.m_MeshIndicesCount)
{
    m_Primitives.shrink_to_fit();

    m_MeshVertexStart = 0;
    m_MeshVertexEnd = 0;
    m_MeshIndexStart = 0;
    m_MeshIndexEnd = 0;
    m_MeshVertexCount = 0;
    m_MeshIndicesCount = 0;
}

const Fleur::Graphics::FLMaterial* Fleur::Graphics::Model::GetMaterial(uint32_t idx) const
{
    if (idx >= m_Materials.size())
        return nullptr;

    return &m_Materials[idx];
}

void Fleur::Graphics::Model::PostCreate(SFLPostCreateInfo&& info)
{
    m_Vertices = std::move(info.m_Vertices);
    m_Indices = std::move(info.m_Indices);
    m_Materials = std::move(info.materials);
    m_WorldTransforms = std::move(info.worldTransforms);
    m_MeshInstance = std::move(info.meshInstance);

    m_Meshes = std::move(info.meshes);

    m_PrimitiveCount = info.primitiveCount;

    m_BoundingBox = info.modelBoundingBox;
}
