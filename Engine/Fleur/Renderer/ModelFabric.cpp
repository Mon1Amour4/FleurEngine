#include "ModelFabric.h"

#include "FleurAllocator.hpp"
#include "Services/ServiceLocator.h"

//======================================================================
// CGLTFModelFabric
Fleur::Graphics::CGLTFModelFabric::CGLTFModelFabric(std::string_view name, const cgltf_data const* data)
    : m_Name(name)
    , m_Data(data)
{
}

Fleur::Graphics::Model* Fleur::Graphics::CGLTFModelFabric::ProcessModel(bool async)
{
    auto assetsManager = Fleur::ServiceLocator::instance().GetService<Fleur::AssetsManager>();

    Fleur::Memory::FleurAllocator<Fleur::Graphics::Model> allocator;
    Model* model = allocator.construct_at(m_Name);

    model->m_MeshCount = static_cast<uint32_t>(m_Data->meshes_count);

    model->m_Meshes.reserve(model->m_MeshCount);
    model->m_Materials.reserve(m_Data->materials_count);

    int textureIdx = MAXINT;
    // std::map<uint32_t, const Texture*> loaded_textures;
    for (size_t i = 0; i < model->m_Materials.capacity(); i++)
    {
        uint32_t solidTextureIdx = 0;

        if ((m_Data->materials + i)->has_pbr_metallic_roughness)
        {
            Fleur::Graphics::Material flMaterial{};

            auto currentMaterial = m_Data->materials + i;
            bool hasTexture = false;
            cgltf_texture* baseColorTexture = currentMaterial->pbr_metallic_roughness.base_color_texture.texture;
            char* textureName = nullptr;
            if (baseColorTexture)
                hasTexture = true;

            if (hasTexture)
            {
                if (baseColorTexture->image->name)
                    textureName = baseColorTexture->image->name;
                else if (baseColorTexture->image->buffer_view->name)
                    textureName = baseColorTexture->image->buffer_view->name;

                if (!baseColorTexture->image->uri && baseColorTexture->image->buffer_view)
                {
                    // Embeded texture
                    auto imageBuffer = baseColorTexture->image->buffer_view;
                    unsigned char* imageData = reinterpret_cast<unsigned char*>(imageBuffer->buffer->data) + imageBuffer->offset;

                    flMaterial.albedo = assetsManager->LoadImageFromMemory(textureName, imageData, static_cast<uint32_t>(imageBuffer->size));
                }
                else if (baseColorTexture->image->uri)
                {
                    // Texture somewhere in folder
                    flMaterial.albedo = assetsManager->MakeHandle<Fleur::Graphics::Image2D>(baseColorTexture->image->uri);
                }
            }
            else
            {
                cgltf_float* color = currentMaterial->pbr_metallic_roughness.base_color_factor;
                int channels = 0;
                for (size_t j = 0; j < 4; j++)
                {
                    if (*(color + j) > 0)
                        ++channels;
                }
                std::string materialName;
                if (currentMaterial->name)
                    materialName = currentMaterial->name;
                else
                    materialName = std::string(m_Name) + "Solid_texture" + std::to_string(solidTextureIdx);
                Color c;
                if (channels == 4)
                    c = Color(*color, *(color + 1), *(color + 2), *(color + 3));
                else if (channels == 3)
                    c = Color(*color, *(color + 1), *(color + 2));
                else if (channels == 2)
                    c = Color(*color, *(color + 1));
                else
                    c = Color(*color);
                flMaterial.albedo = assetsManager->LoadImage2DFromColor(materialName, c, 128, 128);

                ++solidTextureIdx;
            }

            //    // TODO think about passing raw pointer or shared ptr to material
            // ShaderComponentContext ctx{};
            // ctx.albedo_text.second = texture.get();
            // auto material = Material::CreateMaterial(ctx);
            model->m_Materials.push_back(std::move(flMaterial));
            //    loaded_textures.emplace(textureIdx, texture.get());
        }
    }


    for (size_t i = 0; i < model->m_MeshCount; i++)
    {
        auto cgltfMesh = (m_Data->meshes + i);
        for (size_t j = 0; j < cgltfMesh->primitives_count; j++)
        {
            auto primitive = cgltfMesh->primitives[j];
            for (size_t k = 0; k < primitive.attributes_count; k++)
            {
                auto attrib = primitive.attributes[k];
                if (attrib.type == cgltf_attribute_type_position)
                    model->m_ModelVertexCount += static_cast<uint32_t>(attrib.data->count);
            }
            model->m_ModelIndicesCount += static_cast<uint32_t>(primitive.indices->count);
        }
        Model::Mesh& modelMesh = model->m_Meshes.emplace_back();
        modelMesh.m_Primitives.reserve(cgltfMesh->primitives_count);
        modelMesh.m_MeshName = cgltfMesh->name;
        modelMesh.m_MeshVertexStart = model->m_Vertices.size();
        modelMesh.m_MeshIndexStart = model->m_Indices.size();

        model->m_PrimitiveCount += cgltfMesh->primitives_count;
        for (size_t i = 0; i < cgltfMesh->primitives_count; i++)
        {
            cgltf_primitive cgltfPrimitive = cgltfMesh->primitives[i];
            uint32_t materialIdx = static_cast<uint32_t>(cgltfPrimitive.material - m_Data->materials);
            Model::Mesh::Primitive& meshPrimitive = modelMesh.m_Primitives.emplace_back(process_primitive(model, cgltfPrimitive, materialIdx));

            modelMesh.m_MeshVertexCount += meshPrimitive.VertexCount();
            modelMesh.m_MeshIndicesCount += meshPrimitive.IndexCount();
        }
        modelMesh.m_MeshVertexEnd = static_cast<uint32_t>(model->m_Vertices.size());
        modelMesh.m_MeshIndexEnd = static_cast<uint32_t>(model->m_Indices.size());
    }
    model->m_Vertices.reserve(model->m_ModelVertexCount);
    model->m_Indices.reserve(model->m_ModelIndicesCount);

    return model;
}

Fleur::Graphics::Model::Mesh::Primitive Fleur::Graphics::CGLTFModelFabric::process_primitive(Fleur::Graphics::Model* model, cgltf_primitive& cgltfPrimitive,
                                                                                             uint32_t maxIdx)
{
    Fleur::Graphics::Model::Mesh::Primitive meshPrimitive = Fleur::Graphics::Model::Mesh::Primitive();
    meshPrimitive.m_MatIdx = maxIdx;
    meshPrimitive.m_PrimitiveIndicesCount = static_cast<uint32_t>(cgltfPrimitive.indices->count);
    FL_CORE_ASSERT(cgltfPrimitive.type == cgltf_primitive_type_triangles, "Mesh is not triangulated");

    for (size_t i = 0; i < cgltfPrimitive.attributes_count; i++)
    {
        if (cgltfPrimitive.attributes[i].type == cgltf_attribute_type_position)
        {
            meshPrimitive.m_PrimitiveVertexCount = static_cast<uint32_t>(cgltfPrimitive.attributes[i].data->count);
        }
    }
    meshPrimitive.m_PrimitiveVertexStart = static_cast<uint32_t>(model->m_Vertices.size());
    meshPrimitive.m_PrimitiveIndexStart = static_cast<uint32_t>(model->m_Indices.size());
    const cgltf_accessor* primitiveIndicesBuffer = cgltfPrimitive.indices;

    const uint8_t* indexGlobalBuffer = static_cast<const uint8_t*>(primitiveIndicesBuffer->buffer_view->buffer->data);
    size_t primitiveIndeciesStartIdx = primitiveIndicesBuffer->buffer_view->offset + primitiveIndicesBuffer->offset;
    const void* indexData = indexGlobalBuffer + primitiveIndeciesStartIdx;

    const float* positions = nullptr;
    const float* normals = nullptr;
    const float* textcoords = nullptr;

    for (size_t j = 0; j < cgltfPrimitive.attributes_count; j++)
    {
        const cgltf_attribute& attribute = cgltfPrimitive.attributes[j];
        const cgltf_accessor* accessor = attribute.data;

        const uint8_t* attributeGlobalBuffer = static_cast<const uint8_t*>(accessor->buffer_view->buffer->data);
        size_t startIdx = accessor->buffer_view->offset + accessor->offset;
        const float* ptr = reinterpret_cast<const float*>(attributeGlobalBuffer + startIdx);

        if (attribute.type == cgltf_attribute_type_position)
            positions = ptr;
        else if (attribute.type == cgltf_attribute_type_normal)
            normals = ptr;
        else if (attribute.type == cgltf_attribute_type_texcoord)
            textcoords = ptr;
    }

    auto readIndex = [&](size_t idx) -> uint32_t
    {
        if (primitiveIndicesBuffer->component_type == cgltf_component_type_r_16u)
            return reinterpret_cast<const uint16_t*>(indexData)[idx];
        else if (primitiveIndicesBuffer->component_type == cgltf_component_type_r_32u)
            return reinterpret_cast<const uint32_t*>(indexData)[idx];
        return 0;
    };

    std::unordered_map<uint32_t, uint32_t> map;
    for (size_t j = 0; j < primitiveIndicesBuffer->count; ++j)
    {
        uint32_t vi = readIndex(j);
        if (map.contains(vi))
        {
            model->m_Indices.push_back(map[vi]);
            continue;
        }
        SVertexData v{};

        if (positions)
        {
            v.Position.x = positions[vi * 3 + 0];
            v.Position.y = positions[vi * 3 + 1];
            v.Position.z = positions[vi * 3 + 2];
        }
        if (normals)
        {
            v.Normal.x = normals[vi * 3 + 0];
            v.Normal.y = normals[vi * 3 + 1];
            v.Normal.z = normals[vi * 3 + 2];
        }
        if (textcoords)
        {
            v.TexCoord.x = textcoords[vi * 2 + 0];
            v.TexCoord.y = textcoords[vi * 2 + 1];
        }

        model->m_Vertices.push_back(v);
        uint32_t newIndex = static_cast<uint32_t>(model->m_Vertices.size() - 1);
        map[vi] = newIndex;
        model->m_Indices.push_back(newIndex);
    }
    meshPrimitive.m_PrimitiveVertexEnd = static_cast<uint32_t>(model->m_Vertices.size()) - 1;
    meshPrimitive.m_PrimitiveIndexEnd = static_cast<uint32_t>(model->m_Indices.size()) - 1;

    return meshPrimitive;
}
