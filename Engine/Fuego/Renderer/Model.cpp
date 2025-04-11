#include "Model.h"
#include "Renderer.h"

#include <filesystem>

#include "fstream"
#include <assimp/Importer.hpp>  
#include "assimp/scene.h"     
#include <assimp/postprocess.h>

Fuego::Renderer::Model::Model(const aiScene* scene)
    : name(scene->mRootNode->mName.C_Str())
    , mesh_count(scene->mNumMeshes)
    , vertex_count(0)
    , indices_count(0)
{
    meshes.reserve(scene->mNumMeshes);
    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        meshes.emplace_back(std::make_unique<Fuego::Renderer::Model::Mesh>(scene->mMeshes[i], scene->mMaterials, i, vertices, indices));
        vertex_count += scene->mMeshes[i]->mNumVertices;
        indices_count += meshes.back().get()->GetIndicesCount();
    }
}

Fuego::Renderer::Model::Model(Model&& other) noexcept
    : name(std::move(other.name))
    , mesh_count(other.mesh_count)
    , vertex_count(other.vertex_count)
    , meshes(std::move(other.meshes))
{
    other.mesh_count = 0;
    other.vertex_count = 0;
}

Fuego::Renderer::Model& Fuego::Renderer::Model::operator=(Model&& other) noexcept
{
    if (this != &other)
    {
        name = std::move(other.name);
        mesh_count = other.mesh_count;
        vertex_count = other.vertex_count;
        meshes = std::move(other.meshes);

        other.mesh_count = 0;
        other.vertex_count = 0;
    }
    return *this;
}

Fuego::Renderer::Model::Mesh::Mesh(aiMesh* mesh, aiMaterial** materials, uint16_t mesh_index, std::vector<Fuego::Renderer::VertexData>& vertices,
                                   std::vector<uint32_t>& indices)
    : mesh_name(mesh->mName.C_Str())
    , material(materials[mesh->mMaterialIndex]->GetName().C_Str())
    , vertex_count(mesh->mNumVertices)
    , start_index(indices.size() - 1)
    , end_index(0)
    , indices_count(0)
{
    vertices.reserve(vertex_count);
    for (size_t i = 0; i < vertex_count; i++)
    {
        vertices.emplace_back(glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z));
        VertexData& vertex = vertices.back();
        if (mesh->HasNormals())
        {
            vertex.normal.x = mesh->mNormals[i].x;
            vertex.normal.y = mesh->mNormals[i].y;
            vertex.normal.z = mesh->mNormals[i].z;
        }
        // TODO: mesh may contain up to 8 different texture, 
        // use first one for now, later increase textures up to 8?
        if (mesh->HasTextureCoords(mesh_index))
        {
            vertex.textcoord.x = mesh->mTextureCoords[0]->x;
            vertex.textcoord.y = mesh->mTextureCoords[0]->y;
        }
    }
    for (size_t i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (size_t j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
        indices_count += face.mNumIndices;
    }
    end_index = indices.size() - 1;

}
