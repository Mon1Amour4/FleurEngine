#include "Model.h"

#include <filesystem>

// TODO Model must not know about parser
#include "External/cgltf/cgltf.h"
#include "Renderer.h"
#include "fstream"

Fleur::Graphics::Model::Model(std::string_view modelName, cgltf_data* data)
    : m_Name(modelName)
    , m_MeshCount(static_cast<uint32_t>(data->meshes_count))
    , m_ModelVertexCount(0)
    , m_ModelIndicesCount(0)
{
    process_model(data, false);
}

Fleur::Graphics::Model::Model(std::string_view modelName)
    : m_Name(modelName)
    , m_MeshCount(0)
    , m_ModelVertexCount(0)
    , m_ModelIndicesCount(0)
{
}

Fleur::Graphics::Model::Model(Model&& other) noexcept
    : m_Name(std::move(other.m_Name))
    , m_MeshCount(other.m_MeshCount)
    , m_ModelVertexCount(other.m_ModelVertexCount)
    , m_Meshes(std::move(other.m_Meshes))
    , m_ModelIndicesCount(other.m_ModelIndicesCount)
{
    other.m_MeshCount = 0;
    other.m_ModelVertexCount = 0;
}

Fleur::Graphics::Model& Fleur::Graphics::Model::operator=(Model&& other) noexcept
{
    if (this != &other)
    {
        m_Name = std::move(other.m_Name);
        m_MeshCount = other.m_MeshCount;
        m_ModelVertexCount = other.m_ModelVertexCount;
        m_Meshes = std::move(other.m_Meshes);
        m_ModelIndicesCount = other.m_ModelIndicesCount;

        other.m_MeshCount = 0;
        other.m_ModelVertexCount = 0;
        other.m_ModelIndicesCount = 0;
    }
    return *this;
}

Fleur::Graphics::Model::Mesh::Mesh(cgltf_mesh* mesh, const cgltf_material* baseMaterials,
                                   std::vector<Fleur::Graphics::VertexData>& vertices,
                                   std::vector<uint32_t>& indices)
    : m_MeshName(mesh->name)
    , m_MeshVertexStart(static_cast<uint32_t>(vertices.size()))
    , m_MeshVertexEnd(0)
    , m_MeshIndexStart(static_cast<uint32_t>(indices.size()))
    , m_MeshIndexEnd(0)
    , m_MeshVertexCount(0)
    , m_MeshIndicesCount(0)
{
    m_Primitives.reserve(mesh->primitives_count);
    for (size_t i = 0; i < m_Primitives.capacity(); i++)
    {
        uint32_t materialIdx = static_cast<uint32_t>(mesh->primitives[i].material - baseMaterials);
        Primitive& primitive = m_Primitives.emplace_back(&mesh->primitives[i], materialIdx, vertices, indices);
        m_MeshVertexCount += primitive.VertexCount();
        m_MeshIndicesCount += primitive.IndexCount();
    }
    m_MeshVertexEnd = static_cast<uint32_t>(vertices.size());
    m_MeshIndexEnd = static_cast<uint32_t>(indices.size());
}

void Fleur::Graphics::Model::PostLoad(cgltf_data* data)
{
    process_model(data);
}

const Fleur::Graphics::Material* Fleur::Graphics::Model::GetMaterial(uint32_t idx) const
{
    if (idx >= m_Materials.size())
        return nullptr;

    return &m_Materials[idx];
}

void Fleur::Graphics::Model::process_model(cgltf_data* data, bool async)
{
    m_MeshCount = static_cast<uint32_t>(data->meshes_count);

    auto renderer = ServiceLocator::instance().GetService<Fleur::Graphics::Renderer>();
    auto assetsManager = ServiceLocator::instance().GetService<Fleur::AssetsManager>();
    m_Meshes.reserve(m_MeshCount);
    m_Materials.reserve(data->materials_count);

    int textureIdx = MAXINT;
    std::map<uint32_t, const Texture*> loaded_textures;
    for (size_t i = 0; i < m_Materials.capacity(); i++)
    {
        uint32_t solidTextureIdx = 0;
        std::shared_ptr<Fleur::ResourceHandle<Fleur::Graphics::Image2D>> image{nullptr};
        std::shared_ptr<Fleur::Graphics::Texture> texture{nullptr};

        if ((data->materials + i)->has_pbr_metallic_roughness)
        {
            auto currentMaterial = data->materials + i;
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
                    std::string extension = std::filesystem::path(textureName).extension().string();

                    unsigned char* imageData = reinterpret_cast<unsigned char*>(imageBuffer->buffer->data) + imageBuffer->offset;
                    if (async)
                        image = assetsManager->LoadImage2DFromMemoryAsync(textureName, false, imageData, static_cast<uint32_t>(imageBuffer->size));
                    else
                        image = assetsManager->LoadImage2DFromMemory(textureName, false, imageData, static_cast<uint32_t>(imageBuffer->size));
                }
                else if (baseColorTexture->image->uri)
                {
                    // Texture somewhere in folder
                    image = assetsManager->Load<Image2D>(baseColorTexture->image->uri, async);
                }
                texture = renderer->CreateGraphicsResource<Texture>(image->Resource()->Name());
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
                    materialName = m_Name + "Solid_texture" + std::to_string(solidTextureIdx);

                if (channels == 4)
                    texture = renderer->CreateGraphicsResource<Texture>(materialName, Color(*color, *(color + 1), *(color + 2), *(color + 3)), 128, 128);
                else if (channels == 3)
                    texture = renderer->CreateGraphicsResource<Texture>(materialName, Color(*color, *(color + 1), *(color + 2)), 128, 128);
                else if (channels == 2)
                    texture = renderer->CreateGraphicsResource<Texture>(materialName, Color(*color, *(color + 1)), 128, 128);
                else
                    texture = renderer->CreateGraphicsResource<Texture>(materialName, Color(*color), 128, 128);

                ++solidTextureIdx;
            }

            // TODO think about passing raw pointer or shared ptr to material
            ShaderComponentContext ctx{};
            ctx.albedo_text.second = texture.get();
            auto material = Material::CreateMaterial(ctx);
            m_Materials.push_back(std::move(*material));
            loaded_textures.emplace(textureIdx, texture.get());
        }
    }

    for (size_t i = 0; i < m_MeshCount; i++)
    {
        auto mesh = (data->meshes + i);
        for (size_t j = 0; j < mesh->primitives_count; j++)
        {
            auto primitive = mesh->primitives[j];
            for (size_t k = 0; k < primitive.attributes_count; k++)
            {
                auto attrib = primitive.attributes[k];
                if (attrib.type == cgltf_attribute_type_position)
                    m_ModelVertexCount += static_cast<uint32_t>(attrib.data->count);
            }
            m_ModelIndicesCount += static_cast<uint32_t>(primitive.indices->count);
        }
        m_Meshes.emplace_back(mesh, data->materials, m_Vertices, m_Indices);
    }
    m_Vertices.reserve(m_ModelVertexCount);
    m_Indices.reserve(m_ModelIndicesCount);
}

Fleur::Graphics::Model::Primitive::Primitive(const cgltf_primitive* primitive, uint32_t material,
                                             std::vector<Fleur::Graphics::VertexData>& vertices,
                                             std::vector<uint32_t>& indices)
    : m_MatIdx(material)
    , m_PrimitiveVertexCount(0)
    , m_PrimitiveVertexStart(0)
    , m_PrimitiveVertexEnd(0)
    , m_PrimitiveIndexStart(0)
    , m_PrimitiveIndexEnd(0)
    , m_PrimitiveIndicesCount(static_cast<uint32_t>(primitive->indices->count))
{
    FL_CORE_ASSERT(primitive->type == cgltf_primitive_type_triangles, "Mesh is not triangulated");

    for (size_t i = 0; i < primitive->attributes_count; i++)
    {
        if (primitive->attributes[i].type == cgltf_attribute_type_position)
        {
            m_PrimitiveVertexCount = static_cast<uint32_t>(primitive->attributes[i].data->count);
        }
    }

    m_PrimitiveVertexStart = static_cast<uint32_t>(vertices.size());
    m_PrimitiveIndexStart = static_cast<uint32_t>(indices.size());

    const cgltf_accessor* primitiveIndicesBuffer = primitive->indices;

    const uint8_t* indexGlobalBuffer = static_cast<const uint8_t*>(primitiveIndicesBuffer->buffer_view->buffer->data);
    size_t primitiveIndeciesStartIdx = primitiveIndicesBuffer->buffer_view->offset + primitiveIndicesBuffer->offset;
    const void* indexData = indexGlobalBuffer + primitiveIndeciesStartIdx;

    const float* positions = nullptr;
    const float* normals = nullptr;
    const float* textcoords = nullptr;


    for (size_t j = 0; j < primitive->attributes_count; j++)
    {
        const cgltf_attribute& attribute = primitive->attributes[j];
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
            indices.push_back(map[vi]);
            continue;
        }
        VertexData v{};

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

        vertices.push_back(v);
        uint32_t newIndex = static_cast<uint32_t>(vertices.size() - 1);
        map[vi] = newIndex;
        indices.push_back(newIndex);
    }
    m_PrimitiveVertexEnd = static_cast<uint32_t>(vertices.size()) - 1;
    m_PrimitiveIndexEnd = static_cast<uint32_t>(indices.size()) - 1;
}
