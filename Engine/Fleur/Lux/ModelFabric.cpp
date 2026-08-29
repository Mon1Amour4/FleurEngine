#include "ModelFabric.h"

#include <filesystem>
#include <cmath>
#include <numeric>
#include <unordered_map>
#include <vector>

#include "Services/ServiceLocator.h"
#include "TangentGeneration.hpp"

namespace
{

bool IsSupportedFloatComponent(cgltf_component_type componentType)
{
    switch (componentType)
    {
    case cgltf_component_type_r_8:
    case cgltf_component_type_r_8u:
    case cgltf_component_type_r_16:
    case cgltf_component_type_r_16u:
    case cgltf_component_type_r_32u:
    case cgltf_component_type_r_32f:
        return true;
    case cgltf_component_type_invalid:
    case cgltf_component_type_max_enum:
    default:
        return false;
    }
}

bool IsSupportedIndexComponent(cgltf_component_type componentType)
{
    return componentType == cgltf_component_type_r_8u || componentType == cgltf_component_type_r_16u || componentType == cgltf_component_type_r_32u;
}

bool ValidateAttributeAccessor(const cgltf_accessor* accessor, cgltf_type expectedType, const char* attributeName)
{
    if (!accessor)
    {
        FL_CORE_ERROR("[ModelFabric] Missing {0} accessor", attributeName);
        return false;
    }

    if (accessor->type != expectedType)
    {
        FL_CORE_ERROR("[ModelFabric] {0} accessor has unsupported type {1}; expected {2}", attributeName, static_cast<int>(accessor->type),
                      static_cast<int>(expectedType));
        return false;
    }

    if (!IsSupportedFloatComponent(accessor->component_type))
    {
        FL_CORE_ERROR("[ModelFabric] {0} accessor has unsupported component type {1}", attributeName, static_cast<int>(accessor->component_type));
        return false;
    }

    return true;
}

bool ReadAttribute(const cgltf_accessor* accessor, cgltf_size index, cgltf_float* values, cgltf_size componentCount, const char* attributeName)
{
    if (index >= accessor->count || !cgltf_accessor_read_float(accessor, index, values, componentCount))
    {
        FL_CORE_ERROR("[ModelFabric] Failed to read {0} accessor element {1}", attributeName, index);
        return false;
    }

    return true;
}

} // namespace

//======================================================================
// CGLTFModelFabric

std::string Fleur::Graphics::CGLTFModelFabric::s_SolidImageName = "Solid_texture";

Fleur::Graphics::CGLTFModelFabric::CGLTFModelFabric(std::string_view name, const cgltf_data* data)
    : m_Name(name)
    , m_Data(data)
{
}

std::string Fleur::Graphics::CGLTFModelFabric::GetImageAssetKey(const cgltf_image& image) const
{
    const std::filesystem::path modelPath(m_Name);

    if (image.uri && strncmp(image.uri, "data:", 5) != 0)
    {
        std::string decodedUri = image.uri;
        cgltf_decode_uri(decodedUri.data());
        return (modelPath.parent_path() / decodedUri).lexically_normal().string();
    }

    if (m_Data && m_Data->images)
    {
        const size_t imageIndex = static_cast<size_t>(&image - m_Data->images);
        return modelPath.lexically_normal().string() + "#image_" + std::to_string(imageIndex);
    }

    return modelPath.lexically_normal().string() + "#embedded_image";
}

Fleur::Graphics::AssetID Fleur::Graphics::CGLTFModelFabric::LoadImageAsset(const cgltf_image& image, Fleur::AssetsManager& assetsManager, bool srgb)
{
    const std::string imageAssetKey = GetImageAssetKey(image);

    if (!image.uri && image.buffer_view)
    {
        const auto* imageBuffer = image.buffer_view;
        const uint8_t* imageData = cgltf_buffer_view_data(imageBuffer);
        if (!imageData)
            return 0;
        return assetsManager.LoadImage(imageAssetKey, {.imageSource = IMAGE_SOURCE_MEMORY,
                                                   .gammaCorrection = srgb ? GAMMA_CORRECTION_SRGB : GAMMA_CORRECTION_LINEAR,
                                                   .pMemoryData = const_cast<unsigned char*>(imageData),
                                                   .sizeInMemory = static_cast<uint32_t>(imageBuffer->size)})
            .handle.id;
    }

    if (image.uri && strncmp(image.uri, "data:", 5) == 0)
    {
        const char* comma = strchr(image.uri, ',');
        if (comma && comma - image.uri >= 7 && strncmp(comma - 7, ";base64", 7) == 0)
        {
            const char* base64 = comma + 1;
            const cgltf_size encodedSize = strlen(base64);
            cgltf_size decodedSize = encodedSize - encodedSize / 4;
            if (encodedSize >= 2)
            {
                decodedSize -= (base64[encodedSize - 2] == '=');
                decodedSize -= (base64[encodedSize - 1] == '=');
            }

            void* decodedData = nullptr;
            cgltf_options options{};
            if (cgltf_load_buffer_base64(&options, decodedSize, base64, &decodedData) == cgltf_result_success)
            {
                const AssetID imageId = assetsManager.LoadImage(imageAssetKey, {.imageSource = IMAGE_SOURCE_MEMORY,
                                                                              .gammaCorrection = srgb ? GAMMA_CORRECTION_SRGB : GAMMA_CORRECTION_LINEAR,
                                                                              .pMemoryData = static_cast<unsigned char*>(decodedData),
                                                                              .sizeInMemory = static_cast<uint32_t>(decodedSize)})
                                             .handle.id;
                free(decodedData);
                return imageId;
            }
        }
    }

    if (image.uri)
    {
        return assetsManager.LoadImage(imageAssetKey, {.imageSource = IMAGE_SOURCE_DISK,
                                                       .gammaCorrection = srgb ? GAMMA_CORRECTION_SRGB : GAMMA_CORRECTION_LINEAR})
            .handle.id;
    }

    return 0;
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
        Fleur::Graphics::FLMaterial flMaterial{};

        auto currentMaterial = m_Data->materials + i;
        cgltf_texture* baseColorTexture = currentMaterial->pbr_metallic_roughness.base_color_texture.texture;
        memcpy(&flMaterial.baseColorFactor, currentMaterial->pbr_metallic_roughness.base_color_factor, sizeof(flMaterial.baseColorFactor));

        // glTF defaults alphaMode to OPAQUE and alphaCutoff to 0.5.
        switch (currentMaterial->alpha_mode)
        {
        case cgltf_alpha_mode_mask:
            flMaterial.mode = FLAlphaMode::FL_MASK;
            flMaterial.alphaCutoff = currentMaterial->alpha_cutoff;
            break;
        case cgltf_alpha_mode_blend:
            flMaterial.mode = FLAlphaMode::FL_BLEND;
            break;
        case cgltf_alpha_mode_opaque:
        default:
            flMaterial.mode = FLAlphaMode::FL_OPAQUE;
            break;
        }

        if (baseColorTexture && baseColorTexture->image)
        {
            flMaterial.albedo = LoadImageAsset(*baseColorTexture->image, *assetsManager.get());
        }
        else
        {
            const cgltf_float* color = currentMaterial->pbr_metallic_roughness.base_color_factor;
            Color c(color[0], color[1], color[2], color[3]);

            const std::string materialName = std::string(m_Name) + "#material_" + std::to_string(i) + "#" + s_SolidImageName +
                                              std::to_string(c.ToRGBA8());

            flMaterial.albedo = assetsManager->LoadImage(materialName, {.imageSource = IMAGE_SOURCE_COLOR, .color = c}).handle.id;
        }

        if (currentMaterial->normal_texture.texture)
            ProcessNormalTexture(currentMaterial->normal_texture, flMaterial, *assetsManager.get());

        info.materials.push_back(std::move(flMaterial));
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
                                if (attrib.type == cgltf_attribute_type_position && attrib.data)
                                    info.modelVertexCount += static_cast<uint32_t>(attrib.data->count);
                            }
                            if (primitive.indices)
                                info.modelIndicesCount += static_cast<uint32_t>(primitive.indices->count);
                            else
                            {
                                const cgltf_accessor* position = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0);
                                if (position)
                                    info.modelIndicesCount += static_cast<uint32_t>(position->count);
                            }
                        }

                        Model::Mesh& modelMesh = info.meshes.emplace_back();
                        uploadedMeshes.emplace(reinterpret_cast<std::uintptr_t>(node.mesh), info.meshes.size() - 1);
                        modelMesh.m_Primitives.reserve(cgltfMesh->primitives_count);
                        modelMesh.m_MeshName = cgltfMesh->name ? cgltfMesh->name : "";
                        modelMesh.m_MeshVertexStart = info.m_Vertices.size();
                        modelMesh.m_MeshIndexStart = info.m_Indices.size();

                        for (size_t i = 0; i < cgltfMesh->primitives_count; i++)
                        {
                            cgltf_primitive cgltfPrimitive = cgltfMesh->primitives[i];
                            uint32_t materialIdx = static_cast<uint32_t>(cgltfPrimitive.material - m_Data->materials);
                            FLAlphaMode alphaMode = process_alpha_mode(cgltfPrimitive.material->alpha_mode);

                            Model::Mesh::Primitive importedPrimitive = process_primitive(info.m_Vertices, info.m_Indices, cgltfPrimitive, materialIdx, alphaMode);
                            if (importedPrimitive.GetVertexCount() == 0 || importedPrimitive.GetIdxCount() == 0)
                                continue;

                            Model::Mesh::Primitive& meshPrimitive = modelMesh.m_Primitives.emplace_back(std::move(importedPrimitive));
                            ++info.primitiveCount;

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
                    const Fleur::Vec3 meshCorners[8] = {{meshMin.x, meshMin.y, meshMin.z}, {meshMax.x, meshMin.y, meshMin.z}, {meshMax.x, meshMax.y, meshMin.z},
                                                        {meshMin.x, meshMax.y, meshMin.z}, {meshMin.x, meshMin.y, meshMax.z}, {meshMax.x, meshMin.y, meshMax.z},
                                                        {meshMax.x, meshMax.y, meshMax.z}, {meshMin.x, meshMax.y, meshMax.z}};
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
    if (cgltfPrimitive.type != cgltf_primitive_type_triangles)
    {
        FL_CORE_ERROR("[ModelFabric] Rejecting unsupported primitive mode {0}; only triangles are supported", static_cast<int>(cgltfPrimitive.type));
        return meshPrimitive;
    }

    const cgltf_accessor* positionAccessor = nullptr;
    const cgltf_accessor* normalAccessor = nullptr;
    const cgltf_accessor* texcoordAccessor = nullptr;
    const cgltf_accessor* tangentAccessor = nullptr;
    for (size_t i = 0; i < cgltfPrimitive.attributes_count; ++i)
    {
        const cgltf_attribute& attribute = cgltfPrimitive.attributes[i];
        switch (attribute.type)
        {
        case cgltf_attribute_type_position:
            positionAccessor = attribute.data;
            break;
        case cgltf_attribute_type_normal:
            normalAccessor = attribute.data;
            break;
        case cgltf_attribute_type_texcoord:
            if (attribute.index == 0)
                texcoordAccessor = attribute.data;
            break;
        case cgltf_attribute_type_tangent:
            if (attribute.index == 0)
                tangentAccessor = attribute.data;
            break;
        default:
            break;
        }
    }

    if (!ValidateAttributeAccessor(positionAccessor, cgltf_type_vec3, "POSITION"))
        return meshPrimitive;
    if (normalAccessor && !ValidateAttributeAccessor(normalAccessor, cgltf_type_vec3, "NORMAL"))
        return meshPrimitive;
    if (texcoordAccessor && !ValidateAttributeAccessor(texcoordAccessor, cgltf_type_vec2, "TEXCOORD_0"))
        return meshPrimitive;
    if (tangentAccessor && !ValidateAttributeAccessor(tangentAccessor, cgltf_type_vec4, "TANGENT"))
        return meshPrimitive;

    const cgltf_size sourceVertexCount = positionAccessor->count;
    if (sourceVertexCount == 0)
    {
        FL_CORE_ERROR("[ModelFabric] Rejecting primitive with no POSITION vertices");
        return meshPrimitive;
    }

    std::vector<uint32_t> sourceIndices;
    if (cgltfPrimitive.indices)
    {
        const cgltf_accessor* indexAccessor = cgltfPrimitive.indices;
        if (indexAccessor->type != cgltf_type_scalar || !IsSupportedIndexComponent(indexAccessor->component_type))
        {
            FL_CORE_ERROR("[ModelFabric] Rejecting primitive with unsupported index accessor");
            return meshPrimitive;
        }
        sourceIndices.resize(indexAccessor->count);
        for (cgltf_size i = 0; i < indexAccessor->count; ++i)
        {
            const cgltf_size sourceIndex = cgltf_accessor_read_index(indexAccessor, i);
            if (sourceIndex >= sourceVertexCount)
            {
                FL_CORE_ERROR("[ModelFabric] Rejecting primitive with out-of-range vertex index {0}", sourceIndex);
                return meshPrimitive;
            }
            sourceIndices[i] = static_cast<uint32_t>(sourceIndex);
        }
    }
    else
    {
        sourceIndices.resize(sourceVertexCount);
        std::iota(sourceIndices.begin(), sourceIndices.end(), 0u);
    }

    if (sourceIndices.empty() || sourceIndices.size() % 3 != 0)
    {
        FL_CORE_ERROR("[ModelFabric] Rejecting triangle primitive with index count {0}", sourceIndices.size());
        return meshPrimitive;
    }

    // Read the source vertex stream first. This lets us generate normals and
    // tangent space from the complete indexed topology, including shared
    // vertices and non-indexed triangle lists.
    std::vector<SVertexData> sourceVertices(sourceVertexCount);
    std::vector<Fleur::Vec3> tangentAccum(sourceVertexCount, Fleur::Vec3(0.0f));
    std::vector<Fleur::Vec3> bitangentAccum(sourceVertexCount, Fleur::Vec3(0.0f));
    std::vector<Fleur::Vec3> generatedNormalAccum(sourceVertexCount, Fleur::Vec3(0.0f));

    for (cgltf_size sourceIndex = 0; sourceIndex < sourceVertexCount; ++sourceIndex)
    {
        SVertexData& vertex = sourceVertices[sourceIndex];
        cgltf_float values[4]{};
        if (!ReadAttribute(positionAccessor, sourceIndex, values, 3, "POSITION"))
            return meshPrimitive;
        vertex.Position = Fleur::Vec3(values[0], values[1], values[2]);

        if (normalAccessor)
        {
            if (!ReadAttribute(normalAccessor, sourceIndex, values, 3, "NORMAL"))
                return meshPrimitive;
            vertex.Normal = Fleur::Vec3(values[0], values[1], values[2]);
        }
        if (texcoordAccessor)
        {
            if (!ReadAttribute(texcoordAccessor, sourceIndex, values, 2, "TEXCOORD_0"))
                return meshPrimitive;
            vertex.TexCoord = Fleur::Vec2(values[0], values[1]);
        }
        if (tangentAccessor)
        {
            if (!ReadAttribute(tangentAccessor, sourceIndex, values, 4, "TANGENT"))
                return meshPrimitive;
            vertex.Tangent = Fleur::Vec4(values[0], values[1], values[2], values[3]);
        }
    }

    if (!normalAccessor || !tangentAccessor)
    {
        for (size_t triangle = 0; triangle + 2 < sourceIndices.size(); triangle += 3)
        {
            const uint32_t i0 = sourceIndices[triangle + 0];
            const uint32_t i1 = sourceIndices[triangle + 1];
            const uint32_t i2 = sourceIndices[triangle + 2];
            const Fleur::Vec3 edge1 = sourceVertices[i1].Position - sourceVertices[i0].Position;
            const Fleur::Vec3 edge2 = sourceVertices[i2].Position - sourceVertices[i0].Position;
            const Fleur::Vec3 faceNormal = Fleur::Math::cross(edge1, edge2);

            if (!normalAccessor)
            {
                generatedNormalAccum[i0] += faceNormal;
                generatedNormalAccum[i1] += faceNormal;
                generatedNormalAccum[i2] += faceNormal;
            }

            if (!tangentAccessor && texcoordAccessor)
            {
                const auto frame = AccumulateTriangleTangent(
                    sourceVertices[i0].Position, sourceVertices[i1].Position, sourceVertices[i2].Position,
                    sourceVertices[i0].TexCoord, sourceVertices[i1].TexCoord, sourceVertices[i2].TexCoord);
                if (frame.valid)
                {
                    tangentAccum[i0] += frame.tangent;
                    tangentAccum[i1] += frame.tangent;
                    tangentAccum[i2] += frame.tangent;
                    bitangentAccum[i0] += frame.bitangent;
                    bitangentAccum[i1] += frame.bitangent;
                    bitangentAccum[i2] += frame.bitangent;
                }
            }
        }

        if (!normalAccessor)
        {
            for (size_t i = 0; i < sourceVertices.size(); ++i)
            {
                if (Fleur::Math::length(generatedNormalAccum[i]) > 0.0f)
                    sourceVertices[i].Normal = Fleur::Math::normalize(generatedNormalAccum[i]);
            }
        }

        if (!tangentAccessor)
        {
            for (size_t i = 0; i < sourceVertices.size(); ++i)
            {
                const auto tangent = FinalizeTangent(sourceVertices[i].Normal, tangentAccum[i], bitangentAccum[i]);
                if (!tangent)
                {
                    FL_CORE_ERROR("[ModelFabric] Cannot generate tangent: invalid NORMAL at vertex {0}", i);
                    return meshPrimitive;
                }
                sourceVertices[i].Tangent = *tangent;
            }
        }
    }

    std::vector<SVertexData> primitiveVertices;
    std::vector<uint32_t> primitiveIndices;
    primitiveVertices.reserve(sourceVertexCount);
    primitiveIndices.reserve(sourceIndices.size());
    std::unordered_map<uint32_t, uint32_t> sourceToOutput;
    const uint32_t outputVertexBase = static_cast<uint32_t>(vertices.size());

    for (const uint32_t sourceIndex : sourceIndices)
    {
        if (const auto existing = sourceToOutput.find(sourceIndex); existing != sourceToOutput.end())
        {
            primitiveIndices.push_back(outputVertexBase + existing->second);
            continue;
        }

        SVertexData vertex = sourceVertices[sourceIndex];

        const uint32_t outputIndex = static_cast<uint32_t>(primitiveVertices.size());
        sourceToOutput.emplace(sourceIndex, outputIndex);
        primitiveVertices.push_back(vertex);
        primitiveIndices.push_back(outputVertexBase + outputIndex);
        meshPrimitive.m_BoundingBox.UpdateBoundingBox(vertex.Position, vertex.Position);
    }

    meshPrimitive.m_VertexStart = static_cast<uint32_t>(vertices.size());
    meshPrimitive.m_IdxStart = static_cast<uint32_t>(indices.size());
    meshPrimitive.m_VertexCount = static_cast<uint32_t>(primitiveVertices.size());
    meshPrimitive.m_IdxCount = static_cast<uint32_t>(primitiveIndices.size());
    vertices.insert(vertices.end(), primitiveVertices.begin(), primitiveVertices.end());
    indices.insert(indices.end(), primitiveIndices.begin(), primitiveIndices.end());
    meshPrimitive.m_VertexEnd = static_cast<uint32_t>(vertices.size() - 1);
    meshPrimitive.m_IdxEnd = static_cast<uint32_t>(indices.size() - 1);

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

void Fleur::Graphics::CGLTFModelFabric::ProcessNormalTexture(const cgltf_texture_view& normalTexture, Fleur::Graphics::FLMaterial& material,
                                                             Fleur::AssetsManager& assetsManager)
{
    if (!normalTexture.texture || !normalTexture.texture->image)
        return;

    material.normal = LoadImageAsset(*normalTexture.texture->image, assetsManager, false);
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
