#include "Model.h"

#include "Graphics.hpp"
#include "Material.h"

//======================================================================
// Model
Fleur::Graphics::Model::Model(std::string_view modelName)
    : m_Name(modelName)
    , m_MeshCount(0)
    , m_ModelVertexCount(0)
    , m_ModelIndicesCount(0)
{
}

Fleur::Graphics::Model::Model(Model&& other) noexcept
    : m_Name(std::move(other.m_Name))
    , m_MeshCount(other.m_MeshCount)
    , m_ModelVertexCount(other.m_ModelVertexCount)
    , m_Meshes(std::move(other.m_Meshes))
    , m_ModelIndicesCount(other.m_ModelIndicesCount)
    , m_Vertices(std::move(other.m_Vertices))
    , m_Indices(std::move(other.m_Indices))
    , m_Materials(std::move(other.m_Materials))
{
    other.m_MeshCount = 0;
    other.m_ModelVertexCount = 0;
}

Fleur::Graphics::Model& Fleur::Graphics::Model::operator=(Model&& other) noexcept
{
    if (this != &other)
    {
        m_Name = std::move(other.m_Name);
        m_MeshCount = other.m_MeshCount;
        m_ModelVertexCount = other.m_ModelVertexCount;
        m_Meshes = std::move(other.m_Meshes);
        m_ModelIndicesCount = other.m_ModelIndicesCount;

        other.m_MeshCount = 0;
        other.m_ModelVertexCount = 0;
        other.m_ModelIndicesCount = 0;
    }
    return *this;
}

//======================================================================
// Model::Mesh
Fleur::Graphics::Model::Mesh::Mesh()
    : m_Primitives()
    , m_MeshName()
    , m_MeshVertexStart(0)
    , m_MeshVertexEnd(0)
    , m_MeshIndexStart(0)
    , m_MeshIndexEnd(0)
    , m_MeshVertexCount(0)
    , m_MeshIndicesCount(0)
{
}

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

const Fleur::Graphics::Material* Fleur::Graphics::Model::GetMaterial(uint32_t idx) const
{
    if (idx >= m_Materials.size())
        return nullptr;

    return &m_Materials[idx];
}

//======================================================================
// Model::Primitive
Fleur::Graphics::Model::Mesh::Primitive::Primitive()
    : m_MatIdx(0)
    , m_PrimitiveIndexEnd(0)
    , m_PrimitiveIndexStart(0)
    , m_PrimitiveIndicesCount(0)
    , m_PrimitiveVertexStart(0)
    , m_PrimitiveVertexEnd(0)
    , m_PrimitiveVertexCount(0)
{
}
