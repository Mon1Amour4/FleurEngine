#pragma once

#include "glm/ext.hpp"
#include "glm/glm.hpp"

struct aiScene;
struct aiMesh;
struct aiMaterial;
struct cgltf_data;
struct cgltf_mesh;
struct cgltf_material;
struct cgltf_primitive;

namespace Fleur::Graphics
{
struct VertexData;
class Material;

class FLEUR_API Model
{
public:
    class FLEUR_API Primitive
    {
    public:
        Primitive(const cgltf_primitive* primitive, uint32_t material, std::vector<Fleur::Graphics::VertexData>& vertices, std::vector<uint32_t>& indices);
        ~Primitive() = default;

        inline uint32_t VertexCount() const
        {
            return m_PrimitiveVertexCount;
        }
        inline uint32_t IndexCount() const
        {
            return m_PrimitiveIndicesCount;
        }

        inline uint32_t VertexStart() const
        {
            return m_PrimitiveVertexStart;
        }
        inline uint32_t VertexEnd() const
        {
            return m_PrimitiveVertexEnd;
        }

        inline uint32_t IndexStart() const
        {
            return m_PrimitiveIndexStart;
        }
        inline uint32_t IndexEnd() const
        {
            return m_PrimitiveIndexEnd;
        }

        inline uint32_t VertexSize() const
        {
            return m_PrimitiveVertexCount * sizeof(float);
        }
        inline uint32_t IndexSize() const
        {
            return m_PrimitiveIndicesCount * sizeof(uint32_t);
        }

        inline uint32_t MaterialIdx() const
        {
            return m_MatIdx;
        }

    private:
        uint32_t m_MatIdx;

        uint32_t m_PrimitiveVertexStart;
        uint32_t m_PrimitiveVertexEnd;

        uint32_t m_PrimitiveIndexStart;
        uint32_t m_PrimitiveIndexEnd;

        uint32_t m_PrimitiveVertexCount;
        uint32_t m_PrimitiveIndicesCount;
    };

    class FLEUR_API Mesh
    {
    public:
        Mesh(cgltf_mesh* mesh, const cgltf_material* baseMaterials, std::vector<Fleur::Graphics::VertexData>& vertices, std::vector<uint32_t>& indices);
        ~Mesh() = default;

        inline std::string_view Name() const
        {
            return m_MeshName;
        }

        inline const Primitive* Primitives() const
        {
            if (m_Primitives.size() == 0)
                return nullptr;
            return &m_Primitives[0];
        }
        inline uint32_t PrimitivesCount() const
        {
            return m_Primitives.size();
        }

    private:
        std::vector<Primitive> m_Primitives;

        std::string m_MeshName;

        uint32_t m_MeshVertexStart;
        uint32_t m_MeshVertexEnd;
        uint32_t m_MeshIndexStart;
        uint32_t m_MeshIndexEnd;
        uint32_t m_MeshVertexCount;
        uint32_t m_MeshIndicesCount;
    };

    Model(std::string_view modelName, cgltf_data* data);
    Model(std::string_view modelName);

    ~Model() = default;

    Model(Model&& other) noexcept;
    Model& operator=(Model&& other) noexcept;

    inline std::string_view GetName() const
    {
        return m_Name;
    }
    inline uint32_t GetMeshCount() const
    {
        return m_MeshCount;
    }
    inline uint32_t GetVertexCount() const
    {
        return m_ModelVertexCount;
    }
    inline uint32_t GetIndicesCount() const
    {
        return m_ModelIndicesCount;
    }
    inline const VertexData* GetVerticesData() const
    {
        return m_Vertices.data();
    }
    inline const uint32_t* GetIndicesData() const
    {
        return m_Indices.data();
    }
    const std::vector<Fleur::Graphics::Model::Mesh>* GetMeshesPtr() const
    {
        return &m_Meshes;
    }

    void PostLoad(cgltf_data* data);

    const Material* GetMaterial(uint32_t idx) const;

private:
    std::string m_Name;
    uint32_t m_MeshCount;
    uint32_t m_ModelVertexCount;
    uint32_t m_ModelIndicesCount;
    std::vector<Fleur::Graphics::VertexData> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::vector<Model::Mesh> m_Meshes;

    std::vector<Material> m_Materials;

    void process_model(cgltf_data* data, bool async = true);
};

}  // namespace Fleur::Graphics
