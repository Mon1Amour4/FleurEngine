#pragma once

//#include "Renderer.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"

struct aiScene;
struct aiMesh;
struct aiMaterial;


namespace Fuego::Renderer
{
struct VertexData;

class FUEGO_API Model
{
public:
    Model(const aiScene* scene);
    ~Model() = default;

    Model(Model&& other) noexcept;
    Model& operator=(Model&& other) noexcept;

    inline std::string_view GetName() const
    {
        return name;
    }
    inline uint16_t GetMeshCount() const
    {
        return mesh_count;
    }
    inline uint16_t GetVertexCount() const
    {
        return vertex_count;
    }
    inline uint16_t GetIndicesCount() const
    {
        return indices_count;
    }

    inline const float* GetVerticesData() const
    {
        return (float*)vertices.data();
    }
    inline const uint32_t* GetIndicesData() const
    {
        return (uint32_t*)indices.data();
    }

private:
    std::string name;
    uint16_t mesh_count;
    uint32_t vertex_count;
    uint32_t indices_count;
    std::vector<Fuego::Renderer::VertexData> vertices;
    std::vector<uint32_t> indices;

public:
    class FUEGO_API Mesh
    {
    public:
        Mesh(aiMesh* mesh, aiMaterial** materials, uint16_t mesh_index, std::vector<Fuego::Renderer::VertexData>& vertices, std::vector<uint32_t>& indices);
        ~Mesh() = default;

        inline uint16_t GetVertexCount() const
        {
            return vertex_count;
        }
        inline uint16_t GetIndicesCount() const
        {
            return indices_count;
        }

    private:
        std::string mesh_name;
        std::string material;

        uint32_t start_index;
        uint32_t end_index;

        uint16_t vertex_count;
        uint16_t indices_count;
    };

private:
    std::vector<std::unique_ptr<Model::Mesh>> meshes;
};

}  // namespace Fuego::Renderer
