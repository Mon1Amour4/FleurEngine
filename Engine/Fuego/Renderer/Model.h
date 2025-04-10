#pragma once

#include "Renderer.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"

struct aiScene;
struct aiMesh;
struct aiMaterial;

namespace Fuego::Renderer
{
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

private:
    std::string name;
    uint16_t mesh_count;
    uint32_t vertex_count;

public:
    class FUEGO_API Mesh
    {
    public:
        Mesh(aiMesh* mesh, aiMaterial** materials);
        ~Mesh() = default;

        inline unsigned short int GetVertexCount() const
        {
            return vertex_count;
        }

    private:
        std::string mesh_name;
        std::string material;
        unsigned short int vertex_count;
    };

private:
    std::vector<std::unique_ptr<Model::Mesh>> meshes;
};

}  // namespace Fuego::Renderer
