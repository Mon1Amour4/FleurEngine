#include "Model.h"

#include <filesystem>

#include "fstream"
#include <assimp/Importer.hpp>  
#include "assimp/scene.h"     
#include <assimp/postprocess.h>

Fuego::Renderer::Model::Model(const aiScene* scene)
    : name(scene->mRootNode->mName.C_Str())
    , mesh_count(scene->mNumMeshes)
    , vertex_count(0)
{
    meshes.reserve(scene->mNumMeshes);
    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        meshes.emplace_back(std::make_unique<Fuego::Renderer::Model::Mesh>(scene->mMeshes[i], scene->mMaterials));
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

Fuego::Renderer::Model::Mesh::Mesh(aiMesh* mesh, aiMaterial** materials)
    : mesh_name(mesh->mName.C_Str())
    , material(materials[mesh->mMaterialIndex]->GetName().C_Str())
    , vertex_count(mesh->mNumVertices)
{
    
}
