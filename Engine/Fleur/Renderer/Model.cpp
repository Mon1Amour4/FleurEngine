#include "Model.h"

#include <span>

#include "FleurAllocator.hpp"
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

Fleur::Graphics::MeshBuilder Fleur::Graphics::Model::CreateSubmesh(std::string_view meshName)
{
    return Fleur::Graphics::MeshBuilder(meshName, this);
}

Fleur::Graphics::Model* Fleur::Graphics::Model::QuadModel(Fleur::Graphics::Material&& material)
{
    Fleur::Memory::FleurAllocator<Fleur::Graphics::Model> allocator;
    Fleur::Graphics::Model* quadModel = allocator.construct_at("QuadModel");
    quadModel->m_Materials.emplace_back(std::move(material));
    Fleur::Graphics::MeshBuilder builder = quadModel->CreateSubmesh("QuadMesh");
    builder.AddPrimitive(Model::Mesh::Primitive::PrimitiveShape::Quad).Commit();
    return quadModel;
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

//======================================================================
// MeshBuilder
Fleur::Graphics::MeshBuilder::MeshBuilder(std::string_view meshName, Model* model)
    : m_Name(meshName)
    , m_Model(model)
    , m_Mesh(&m_Model->m_Meshes.emplace_back())
{
    m_Mesh->m_MeshIndexStart = m_Model->m_Indices.size();
    m_Mesh->m_MeshVertexStart = m_Model->m_Vertices.size();
    m_Model->m_MeshCount += 1;
}

Fleur::Graphics::MeshBuilder& Fleur::Graphics::MeshBuilder::AddPrimitive(Model::Mesh::Primitive::PrimitiveShape shape)
{
    if (shape == Model::Mesh::Primitive::PrimitiveShape::Quad)
    {
        Model::Mesh::Primitive& primitive = m_Mesh->m_Primitives.emplace_back();

        std::vector<Fleur::Graphics::SVertexData, Fleur::Memory::FleurAllocator<Fleur::Graphics::SVertexData>> vertices{
            {glm::vec3(-0.5f, 0.5f, 0)}, {glm::vec3(-0.5f, -0.5f, 0)}, {glm::vec3(0.5f, 0.5f, 0)}, {glm::vec3(0.5f, -0.5f, 0)}};

        primitive.m_PrimitiveVertexStart = m_Model->m_Vertices.size();
        m_Model->m_Vertices.reserve(m_Model->m_Vertices.size() + vertices.size());
        m_Model->m_Vertices.insert(m_Model->m_Vertices.end(), vertices.begin(), vertices.end());
        primitive.m_PrimitiveVertexCount = vertices.size();
        primitive.m_PrimitiveVertexEnd = m_Model->m_Vertices.size();
        m_Mesh->m_MeshVertexCount += vertices.size();
        m_Mesh->m_MeshVertexEnd = m_Mesh->m_MeshVertexStart + m_Mesh->m_MeshVertexCount;

        std::vector<uint32_t, Fleur::Memory::FleurAllocator<uint32_t>> indices{0, 1, 2, 2, 1, 3, 0, 2, 3};

        primitive.m_PrimitiveIndexStart = m_Model->m_Indices.size();
        m_Model->m_Indices.reserve(m_Model->m_Indices.size() + indices.size());
        m_Model->m_Indices.insert(m_Model->m_Indices.end(), indices.begin(), indices.end());
        primitive.m_PrimitiveIndicesCount = indices.size();
        primitive.m_PrimitiveIndexEnd = m_Model->m_Indices.size();
        m_Mesh->m_MeshIndicesCount += indices.size();
        m_Mesh->m_MeshIndexEnd = m_Mesh->m_MeshIndexStart + m_Mesh->m_MeshIndicesCount;

        primitive.m_MatIdx = 0;
    }
    return *this;
}

void Fleur::Graphics::MeshBuilder::Commit()
{
    Fleur::Memory::FleurAllocator<Fleur::Graphics::MeshBuilder> allocator;
    allocator.deallocate(this, 1);
}
