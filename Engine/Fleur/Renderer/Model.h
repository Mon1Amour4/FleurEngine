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
        Mesh(cgltf_mesh* mesh, const cgltf_material* base_materials, std::vector<Fleur::Graphics::VertexData>& vertices, std::vector<uint32_t>& indices);
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

    Model(std::string_view model_name, cgltf_data* data);
    Model(std::string_view model_name);

    ~Model() = default;

    Model(Model&& other) noexcept;
    Model& operator=(Model&& other) noexcept;

    inline std::string_view GetName() const
    {
        return name;
    }
    inline uint32_t GetMeshCount() const
    {
        return mesh_count;
    }
    inline uint32_t GetVertexCount() const
    {
        return model_vertex_count;
    }
    inline uint32_t GetIndicesCount() const
    {
        return model_indices_count;
    }
    inline const VertexData* GetVerticesData() const
    {
        return vertices.data();
    }
    inline const uint32_t* GetIndicesData() const
    {
        return indices.data();
    }
    const std::vector<Fleur::Graphics::Model::Mesh>* GetMeshesPtr() const
    {
        return &meshes;
    }

    void PostLoad(cgltf_data* data);

    const Material* GetMaterial(uint32_t idx) const;

private:
    std::string name;
    uint32_t mesh_count;
    uint32_t model_vertex_count;
    uint32_t model_indices_count;
    std::vector<Fleur::Graphics::VertexData> vertices;
    std::vector<uint32_t> indices;
    std::vector<Model::Mesh> meshes;

    std::vector<Material> materials;

    void process_model(cgltf_data* data, bool async = true);
};

}  // namespace Fleur::Graphics
