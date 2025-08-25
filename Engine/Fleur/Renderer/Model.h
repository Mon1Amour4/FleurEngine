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
            return primitive_vertex_count;
        }
        inline uint32_t IndexCount() const
        {
            return primitive_indices_count;
        }

        inline uint32_t VertexStart() const
        {
            return primitive_vertex_start;
        }
        inline uint32_t VertexEnd() const
        {
            return primitive_vertex_end;
        }

        inline uint32_t IndexStart() const
        {
            return primitive_index_start;
        }
        inline uint32_t IndexEnd() const
        {
            return primitive_index_end;
        }

        inline uint32_t VertexSize() const
        {
            return primitive_vertex_count * sizeof(float);
        }
        inline uint32_t IndexSize() const
        {
            return primitive_indices_count * sizeof(uint32_t);
        }

        inline uint32_t MaterialIdx() const
        {
            return mat_idx;
        }

    private:
        uint32_t mat_idx;

        uint32_t primitive_vertex_start;
        uint32_t primitive_vertex_end;

        uint32_t primitive_index_start;
        uint32_t primitive_index_end;

        uint32_t primitive_vertex_count;
        uint32_t primitive_indices_count;
    };

    class FLEUR_API Mesh
    {
    public:
        Mesh(cgltf_mesh* mesh, const cgltf_material* baseMaterials, std::vector<Fleur::Graphics::VertexData>& vertices, std::vector<uint32_t>& indices);
        ~Mesh() = default;

        inline std::string_view Name() const
        {
            return mesh_name;
        }

        inline const Primitive* Primitives() const
        {
            if (primitives.size() == 0)
                return nullptr;
            return &primitives[0];
        }
        inline uint32_t PrimitivesCount() const
        {
            return primitives.size();
        }

    private:
        std::vector<Primitive> primitives;

        std::string mesh_name;

        uint32_t mesh_vertex_start;
        uint32_t mesh_vertex_end;
        uint32_t mesh_index_start;
        uint32_t mesh_index_end;
        uint32_t mesh_vertex_count;
        uint32_t mesh_indices_count;
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
