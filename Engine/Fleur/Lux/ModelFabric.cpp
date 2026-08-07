#include "ModelFabric.h"

#include "Services/ServiceLocator.h"

//======================================================================
// CGLTFModelFabric
Fleur::Graphics::CGLTFModelFabric::CGLTFModelFabric(std::string_view name, const cgltf_data const* data)
    : m_Name(name)
    , m_Data(data)
{
}

Fleur::Graphics::Model::SFLPostCreateInfo Fleur::Graphics::CGLTFModelFabric::ProcessData(bool async)
{
    UNUSED(async);

    Fleur::Graphics::Model::SFLPostCreateInfo info{};
    auto assetsManager = Fleur::ServiceLocator::instance().GetService<Fleur::AssetsManager>();

    info.meshes.reserve(m_Data->meshes_count);
    info.materials.reserve(m_Data->materials_count);
    info.worldTransforms.reserve(m_Data->nodes_count);

    for (size_t i = 0; i < m_Data->materials_count; i++)
    {
        if ((m_Data->materials + i)->has_pbr_metallic_roughness)
        {
            Fleur::Graphics::FLMaterial flMaterial{};

            auto currentMaterial = m_Data->materials + i;
            bool hasTexture = false;
            cgltf_texture* baseColorTexture = currentMaterial->pbr_metallic_roughness.base_color_texture.texture;
            std::string textureName;
            if (baseColorTexture)
                hasTexture = true;

            // Alpha Mode
            // - cgltf_alpha_mode_opaque - alpha values (transparency) from textures are ignored
            // - cgltf_alpha_mode_mask  - it tells your rendering engine to evaluate the material's alpha (transparency) channel against a specific
            // threshold value, known as alpha_cutoff
            // - cgltf_alpha_mode_blend - indicates a material should use standard, linear-interpolated alpha blending (transparency).
            cgltf_alpha_mode alphaMode = currentMaterial->alpha_mode;
            if (alphaMode == cgltf_alpha_mode_opaque)
            {
                flMaterial.mode = FLAlphaMode::FL_OPAQUE;
            }
            else if (alphaMode == cgltf_alpha_mode_mask)
            {
                flMaterial.mode = FLAlphaMode::FL_MASK;
                flMaterial.alphaCutoff = currentMaterial->alpha_cutoff;
            }
            else if (alphaMode == cgltf_alpha_mode_blend)
            {
                flMaterial.mode = FLAlphaMode::FL_BLEND;
                memcpy((void*)&flMaterial.baseColorFactor, (void*)&currentMaterial->pbr_metallic_roughness.base_color_factor, 4 * sizeof(float));
            }

            static uint64_t embededTextureIdx = 0;
            if (hasTexture)
            {
                cgltf_image* image = baseColorTexture->image;

                // Cache-key name. Null-safe: buffer_view is null for external-file
                // images, so the old `buffer_view->name` would crash on URI models.
                if (image->name)
                    textureName = std::string(image->name);
                else if (image->buffer_view && image->buffer_view->name)
                    textureName = std::string(image->buffer_view->name);
                else
                {
                    if (image->uri)
                        textureName = std::string(image->uri);
                    else
                    {
                        textureName = "unique_embedded_texture_" + std::to_string(embededTextureIdx);
                        embededTextureIdx++;
                    }
                }

                if (!image->uri && image->buffer_view)
                {
                    // Embedded in a binary buffer (GLB)
                    auto imageBuffer = image->buffer_view;
                    unsigned char* imageData = reinterpret_cast<unsigned char*>(imageBuffer->buffer->data) + imageBuffer->offset;
                    flMaterial.albedo = assetsManager
                                            ->LoadImage(textureName, {.imageSource = IMAGE_SOURCE_MEMORY,
                                                                      .pMemoryData = imageData,
                                                                      .sizeInMemory = static_cast<uint32_t>(imageBuffer->size)})
                                            .handle.id;
                }
                else if (image->uri && strncmp(image->uri, "data:", 5) == 0)
                {
                    // Embedded base64 data URI:  data:[mime];base64,<DATA>
                    const char* comma = strchr(image->uri, ',');
                    if (comma && comma - image->uri >= 7 && strncmp(comma - 7, ";base64", 7) == 0)
                    {
                        const char* base64 = comma + 1;
                        cgltf_size b64size = strlen(base64);
                        cgltf_size decodedSize = b64size - b64size / 4;  // 3 bytes / 4 chars
                        if (b64size >= 2)
                        {
                            decodedSize -= (base64[b64size - 2] == '=');
                            decodedSize -= (base64[b64size - 1] == '=');
                        }

                        void* decoded = nullptr;
                        cgltf_options options{};
                        if (cgltf_load_buffer_base64(&options, decodedSize, base64, &decoded) == cgltf_result_success)
                        {
                            flMaterial.albedo = assetsManager
                                                    ->LoadImage(textureName, {.imageSource = IMAGE_SOURCE_MEMORY,
                                                                              .pMemoryData = static_cast<unsigned char*>(decoded),
                                                                              .sizeInMemory = static_cast<uint32_t>(decodedSize)})
                                                    .handle.id;
                            free(decoded);  // cgltf allocates via options' allocator (default malloc)
                        }
                    }
                }
                else if (image->uri)
                {
                    // External file, resolved relative to the model's directory
                    cgltf_decode_uri(image->uri);  // percent-decode (%20 etc.) in place
                    std::string path = /* m_ModelDir + "/" +*/ image->uri;
                    flMaterial.albedo = assetsManager->LoadImage(path).handle.id;
                }
            }
            else
            {
                cgltf_float* color = currentMaterial->pbr_metallic_roughness.base_color_factor;
                Color c(color[0], color[1], color[2], color[3]);

                std::string materialName;
                if (currentMaterial->name)
                    materialName = currentMaterial->name;
                else
                    materialName = std::string(m_Name) + "Solid_texture" + std::to_string(c.ToRGBA8());

                flMaterial.albedo = assetsManager->LoadImage(materialName, {.imageSource = IMAGE_SOURCE_COLOR, .color = c}).handle.id;
            }

            info.materials.push_back(std::move(flMaterial));
        }
    }

    PrintNodeMeshes(m_Data);

    uint32_t meshesCount = m_Data->meshes_count;
    uint32_t nodesCount = m_Data->nodes_count;
    std::unordered_map<std::uintptr_t, uint32_t> uploadedMeshes;
    for (size_t i = 0; i < meshesCount; i++)
    {
        const auto cgltfMesh = &m_Data->meshes[i];
        for (size_t j = 0; j < nodesCount; j++)
        {
            auto& node = m_Data->nodes[j];
            if (node.mesh)
            {
                if (node.mesh == cgltfMesh)
                {
                    cgltf_float matrix[16];
                    cgltf_node_transform_world(&node, matrix);
                    info.worldTransforms.emplace_back(Fleur::Math::make_mat4(matrix));

                    if (!uploadedMeshes.contains(reinterpret_cast<std::uintptr_t>(node.mesh)))
                    {
                        for (size_t j = 0; j < cgltfMesh->primitives_count; j++)
                        {
                            auto primitive = cgltfMesh->primitives[j];
                            for (size_t k = 0; k < primitive.attributes_count; k++)
                            {
                                auto attrib = primitive.attributes[k];
                                if (attrib.type == cgltf_attribute_type_position)
                                    info.modelVertexCount += static_cast<uint32_t>(attrib.data->count);
                            }
                            info.modelIndicesCount += static_cast<uint32_t>(primitive.indices->count);
                        }

                        Model::Mesh& modelMesh = info.meshes.emplace_back();
                        uploadedMeshes.emplace(reinterpret_cast<std::uintptr_t>(node.mesh), info.meshes.size() - 1);
                        modelMesh.m_Primitives.reserve(cgltfMesh->primitives_count);
                        modelMesh.m_MeshName = cgltfMesh->name;
                        modelMesh.m_MeshVertexStart = info.m_Vertices.size();
                        modelMesh.m_MeshIndexStart = info.m_Indices.size();

                        info.primitiveCount += cgltfMesh->primitives_count;
                        for (size_t i = 0; i < cgltfMesh->primitives_count; i++)
                        {
                            cgltf_primitive cgltfPrimitive = cgltfMesh->primitives[i];
                            uint32_t materialIdx = static_cast<uint32_t>(cgltfPrimitive.material - m_Data->materials);
                            FLAlphaMode alphaMode = process_alpha_mode(cgltfPrimitive.material->alpha_mode);

                            Model::Mesh::Primitive& meshPrimitive =
                                modelMesh.m_Primitives.emplace_back(process_primitive(info.m_Vertices, info.m_Indices, cgltfPrimitive, materialIdx, alphaMode));

                            BoundingBox primitiveBoundingBox = meshPrimitive.GetBoundingBox();
                            modelMesh.m_BoundingBox.UpdateBoundingBox(primitiveBoundingBox.GetMin(), primitiveBoundingBox.GetMax());

                            modelMesh.m_MeshVertexCount += meshPrimitive.GetVertexCount();
                            modelMesh.m_MeshIndicesCount += meshPrimitive.GetIdxCount();
                        }

                        modelMesh.m_MeshVertexEnd = static_cast<uint32_t>(info.m_Vertices.size());
                        modelMesh.m_MeshIndexEnd = static_cast<uint32_t>(info.m_Indices.size());

                        info.meshInstance.emplace_back(info.meshes.size() - 1, 0, info.worldTransforms.size() - 1);

                    }

                    const uint32_t meshIndex = uploadedMeshes.at(reinterpret_cast<std::uintptr_t>(node.mesh));
                    const BoundingBox meshBoundingBox = info.meshes[meshIndex].GetBoundingBox();
                    const Fleur::Vec3 meshMin = meshBoundingBox.GetMin();
                    const Fleur::Vec3 meshMax = meshBoundingBox.GetMax();
                    const Fleur::Mat4& nodeTransform = info.worldTransforms.back();
                    const Fleur::Vec3 meshCorners[8] = {
                        {meshMin.x, meshMin.y, meshMin.z}, {meshMax.x, meshMin.y, meshMin.z}, {meshMax.x, meshMax.y, meshMin.z}, {meshMin.x, meshMax.y, meshMin.z},
                        {meshMin.x, meshMin.y, meshMax.z}, {meshMax.x, meshMin.y, meshMax.z}, {meshMax.x, meshMax.y, meshMax.z}, {meshMin.x, meshMax.y, meshMax.z}};
                    for (const Fleur::Vec3& corner : meshCorners)
                    {
                        const Fleur::Vec3 worldCorner = Fleur::Vec3(nodeTransform * Fleur::Vec4(corner, 1.0f));
                        info.modelBoundingBox.UpdateBoundingBox(worldCorner, worldCorner);
                    }

                    info.meshInstance.back().drawCount++;
                }
            }
        }
    }

    return info;
}

Fleur::Graphics::Model::Mesh::Primitive Fleur::Graphics::CGLTFModelFabric::process_primitive(OUT std::vector<Fleur::Graphics::SVertexData>& vertices,
                                                                                             OUT std::vector<uint32_t>& indices,
                                                                                             IN cgltf_primitive& cgltfPrimitive, uint32_t maxIdx,
                                                                                             FLAlphaMode alphaMode)
{
    Fleur::Graphics::Model::Mesh::Primitive meshPrimitive = Fleur::Graphics::Model::Mesh::Primitive();
    meshPrimitive.m_MatIdx = maxIdx;
    meshPrimitive.m_AlphaMode = alphaMode;
    meshPrimitive.m_IdxCount = static_cast<uint32_t>(cgltfPrimitive.indices->count);
    FL_CORE_ASSERT(cgltfPrimitive.type == cgltf_primitive_type_triangles, "Mesh is not triangulated");

    for (size_t i = 0; i < cgltfPrimitive.attributes_count; i++)
    {
        if (cgltfPrimitive.attributes[i].type == cgltf_attribute_type_position)
        {
            meshPrimitive.m_VertexCount = static_cast<uint32_t>(cgltfPrimitive.attributes[i].data->count);
        }
    }
    meshPrimitive.m_VertexStart = static_cast<uint32_t>(vertices.size());
    meshPrimitive.m_IdxStart = static_cast<uint32_t>(indices.size());

    bool isUnpackedIndices = false;
    const cgltf_accessor* primitiveIndicesBuffer = cgltfPrimitive.indices;
    std::vector<uint32_t> unpackedIndices;
    if (primitiveIndicesBuffer->component_type != cgltf_component_type_r_32u)
    {
        unpackedIndices.resize(primitiveIndicesBuffer->count);
        isUnpackedIndices = true;
        uint32_t unpackedCount = primitiveIndicesBuffer->count;
        cgltf_accessor_unpack_indices(primitiveIndicesBuffer, unpackedIndices.data(), sizeof(uint32_t), unpackedCount);
    }
    const uint32_t* indexGlobalBuffer = nullptr;
    if (isUnpackedIndices)
        indexGlobalBuffer = unpackedIndices.data();
    else
        indexGlobalBuffer = static_cast<const uint32_t*>(primitiveIndicesBuffer->buffer_view->buffer->data);

    size_t primitiveIndeciesStartIdx =
        (primitiveIndicesBuffer->buffer_view->offset + primitiveIndicesBuffer->offset) / cgltf_component_size(primitiveIndicesBuffer->component_type);

    const void* indexData = nullptr;
    if (!isUnpackedIndices)
        indexData = indexGlobalBuffer + primitiveIndeciesStartIdx;
    else
        indexData = unpackedIndices.data();

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

    std::unordered_map<uint32_t, uint32_t> map;
    for (size_t j = 0; j < primitiveIndicesBuffer->count; ++j)
    {
        uint32_t vi = reinterpret_cast<const uint32_t*>(indexData)[j];
        if (map.contains(vi))
        {
            indices.push_back(map[vi]);
            continue;
        }
        SVertexData v{};

        if (positions)
        {
            v.Position.x = positions[vi * 3 + 0];
            v.Position.y = positions[vi * 3 + 1];
            v.Position.z = positions[vi * 3 + 2];

            meshPrimitive.m_BoundingBox.UpdateBoundingBox(v.Position, v.Position);
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

        vertices.push_back(v);
        uint32_t newIndex = static_cast<uint32_t>(vertices.size() - 1);
        map[vi] = newIndex;
        indices.push_back(newIndex);
    }
    meshPrimitive.m_VertexEnd = static_cast<uint32_t>(vertices.size()) - 1;
    meshPrimitive.m_IdxEnd = static_cast<uint32_t>(indices.size()) - 1;

    return meshPrimitive;
}

Fleur::Graphics::FLAlphaMode Fleur::Graphics::CGLTFModelFabric::process_alpha_mode(cgltf_alpha_mode mode)
{
    switch (mode)
    {
    case cgltf_alpha_mode_opaque:
        return FLAlphaMode::FL_OPAQUE;
        break;
    case cgltf_alpha_mode_mask:
        return FLAlphaMode::FL_MASK;
        break;
    case cgltf_alpha_mode_blend:
        return FLAlphaMode::FL_BLEND;
        break;
    case cgltf_alpha_mode_max_enum:
        break;
    default:
        return FLAlphaMode::FL_OPAQUE;
        break;
    }
}

void Fleur::Graphics::CGLTFModelFabric::PrintNodeMeshes(const cgltf_data* data)
{
    if (!data)
        return;

    for (cgltf_size i = 0; i < data->nodes_count; ++i)
    {
        const cgltf_node& node = data->nodes[i];

       // std::cout << "node[" << i << "] ";

        if (!node.mesh)
        {
         //   std::cout << "mesh: nullptr\n";
            continue;
        }

        auto meshIndex = node.mesh - data->meshes;

        // std::cout << "node_addr: " << &node << " mesh_addr: " << node.mesh << " mesh_index: " << meshIndex << " node_name: " << (node.name ? node.name :
        // "null")
        //           << " mesh_name: " << (node.mesh->name ? node.mesh->name : "null") << "\n";
    }
}
