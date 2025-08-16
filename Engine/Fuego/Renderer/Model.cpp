#include "Model.h"

#include <filesystem>

#include "External/cgltf/cgltf.h"
#include "Renderer.h"
#include "fstream"

Fuego::Graphics::Model::Model(std::string_view model_name, cgltf_data* data)
    : name(model_name)
    , mesh_count(data->meshes_count)
    , model_vertex_count(0)
    , model_indices_count(0)
{
    process_model(data, false);
}

Fuego::Graphics::Model::Model(std::string_view model_name)
    : name(model_name)
    , mesh_count(0)
    , model_vertex_count(0)
    , model_indices_count(0)
{
}

Fuego::Graphics::Model::Model(Model&& other) noexcept
    : name(std::move(other.name))
    , mesh_count(other.mesh_count)
    , model_vertex_count(other.model_vertex_count)
    , meshes(std::move(other.meshes))
    , model_indices_count(other.model_indices_count)
{
    other.mesh_count = 0;
    other.model_vertex_count = 0;
}

Fuego::Graphics::Model& Fuego::Graphics::Model::operator=(Model&& other) noexcept
{
    if (this != &other)
    {
        name = std::move(other.name);
        mesh_count = other.mesh_count;
        model_vertex_count = other.model_vertex_count;
        meshes = std::move(other.meshes);
        model_indices_count = other.model_indices_count;

        other.mesh_count = 0;
        other.model_vertex_count = 0;
        other.model_indices_count = 0;
    }
    return *this;
}

Fuego::Graphics::Model::Mesh::Mesh(cgltf_mesh* mesh, const cgltf_material* base_materials, std::vector<Fuego::Graphics::VertexData>& vertices,
                                   std::vector<uint32_t>& indices)
    : mesh_name(mesh->name)
    , mesh_vertex_start(vertices.size())
    , mesh_vertex_end(0)
    , mesh_index_start(indices.size())
    , mesh_index_end(0)
    , mesh_vertex_count(0)
    , mesh_indices_count(0)
{
    primitives.reserve(mesh->primitives_count);
    for (size_t i = 0; i < primitives.capacity(); i++)
    {
        uint32_t mat_idx = static_cast<uint32_t>(mesh->primitives[i].material - base_materials);
        Primitive& primitive = primitives.emplace_back(&mesh->primitives[i], mat_idx, vertices, indices);
        mesh_vertex_count += primitive.VertexCount();
        mesh_indices_count += primitive.IndexCount();
    }
    mesh_vertex_end = vertices.size();
    mesh_index_end = indices.size();
}

void Fuego::Graphics::Model::PostLoad(cgltf_data* data)
{
    process_model(data);
}

const Fuego::Graphics::Material* Fuego::Graphics::Model::GetMaterial(uint32_t idx) const
{
    if (idx >= materials.size())
        return nullptr;

    return &materials[idx];
}

void Fuego::Graphics::Model::process_model(cgltf_data* data, bool async)
{
    mesh_count = data->meshes_count;

    auto renderer = ServiceLocator::instance().GetService<Fuego::Graphics::Renderer>();
    auto assets_manager = ServiceLocator::instance().GetService<Fuego::AssetsManager>();
    meshes.reserve(mesh_count);
    materials.reserve(data->materials_count);

    int texture_index = MAXINT;
    std::map<uint32_t, const Texture*> loaded_textures;
    for (size_t i = 0; i < materials.capacity(); i++)
    {
        uint32_t solid_texture_idx = 0;
        std::shared_ptr<Fuego::ResourceHandle<Fuego::Graphics::Image2D>> image{nullptr};
        std::shared_ptr<Fuego::Graphics::Texture> texture{nullptr};

        if ((data->materials + i)->has_pbr_metallic_roughness)
        {
            auto current_material = data->materials + i;
            bool has_texture = false;
            cgltf_texture* base_color_texture = current_material->pbr_metallic_roughness.base_color_texture.texture;
            char* texture_name = nullptr;
            if (base_color_texture)
                has_texture = true;

            if (has_texture)
            {
                if (base_color_texture->image->name)
                    texture_name = base_color_texture->image->name;
                else if (base_color_texture->image->buffer_view->name)
                    texture_name = base_color_texture->image->buffer_view->name;

                if (!base_color_texture->image->uri && base_color_texture->image->buffer_view)
                {
                    // Embeded texture
                    auto image_buffer = base_color_texture->image->buffer_view;
                    std::string extension = std::filesystem::path(texture_name).extension().string();

                    unsigned char* image_data = reinterpret_cast<unsigned char*>(image_buffer->buffer->data) + image_buffer->offset;
                    if (async)
                        image = assets_manager->LoadImage2DFromMemoryAsync(texture_name, false, image_data, image_buffer->size);
                    else
                        image = assets_manager->LoadImage2DFromMemory(texture_name, false, image_data, image_buffer->size);
                }
                else if (base_color_texture->image->uri)
                {
                    // Texture somewhere in folder
                    image = assets_manager->Load<Image2D>(base_color_texture->image->uri, async);
                }
                texture = renderer->CreateGraphicsResource<Texture>(image->Resource()->Name());
            }
            else
            {
                cgltf_float* color = current_material->pbr_metallic_roughness.base_color_factor;
                int channels = 0;
                for (size_t i = 0; i < 4; i++)
                {
                    if (*(color + i) > 0)
                        ++channels;
                }
                std::string material_name;
                if (current_material->name)
                    material_name = current_material->name;
                else
                    material_name = name + "Solid_texture" + std::to_string(solid_texture_idx);

                if (channels == 4)
                    texture = renderer->CreateGraphicsResource<Texture>(material_name, Color(*color, *(color + 1), *(color + 2), *(color + 3)), 128, 128);
                else if (channels == 3)
                    texture = renderer->CreateGraphicsResource<Texture>(material_name, Color(*color, *(color + 1), *(color + 2)), 128, 128);
                else if (channels == 2)
                    texture = renderer->CreateGraphicsResource<Texture>(material_name, Color(*color, *(color + 1)), 128, 128);
                else
                    texture = renderer->CreateGraphicsResource<Texture>(material_name, Color(*color), 128, 128);

                ++solid_texture_idx;
            }

            // TODO think about passing raw pointer or shared ptr to material
            ShaderComponentContext ctx{};
            ctx.albedo_text.second = texture.get();
            auto material = Material::CreateMaterial(ctx);
            materials.push_back(std::move(*material));
            loaded_textures.emplace(texture_index, texture.get());
        }
    }

    for (size_t i = 0; i < mesh_count; i++)
    {
        auto mesh = (data->meshes + i);
        for (size_t j = 0; j < mesh->primitives_count; j++)
        {
            auto primitive = mesh->primitives[j];
            for (size_t k = 0; k < primitive.attributes_count; k++)
            {
                auto attrib = primitive.attributes[k];
                if (attrib.type == cgltf_attribute_type_position)
                    model_vertex_count += attrib.data->count;
            }
            model_indices_count += primitive.indices->count;
        }
        Mesh* emplaced_mesh = &meshes.emplace_back(mesh, data->materials, vertices, indices);
    }
    vertices.reserve(model_vertex_count);
    indices.reserve(model_indices_count);
}

Fuego::Graphics::Model::Primitive::Primitive(const cgltf_primitive* primitive, uint32_t material, std::vector<Fuego::Graphics::VertexData>& vertices,
                                             std::vector<uint32_t>& indices)
    : mat_idx(material)
    , primitive_vertex_count(0)
    , primitive_vertex_start(0)
    , primitive_vertex_end(0)
    , primitive_index_start(0)
    , primitive_index_end(0)
    , primitive_indices_count(primitive->indices->count)
{
    FU_CORE_ASSERT(primitive->type == cgltf_primitive_type_triangles, "Mesh is not triangulated");

    for (size_t i = 0; i < primitive->attributes_count; i++)
    {
        if (primitive->attributes[i].type == cgltf_attribute_type_position)
        {
            primitive_vertex_count = primitive->attributes[i].data->count;
        }
    }

    primitive_vertex_start = vertices.size();
    primitive_index_start = indices.size();

    const cgltf_accessor* primitive_indices_buffer = primitive->indices;

    const uint8_t* index_global_buffer = static_cast<const uint8_t*>(primitive_indices_buffer->buffer_view->buffer->data);
    size_t primitive_indecies_start_idx = primitive_indices_buffer->buffer_view->offset + primitive_indices_buffer->offset;
    const void* index_data = index_global_buffer + primitive_indecies_start_idx;

    const float* positions = nullptr;
    const float* normals = nullptr;
    const float* textcoords = nullptr;


    for (size_t j = 0; j < primitive->attributes_count; j++)
    {
        const cgltf_attribute& attribute = primitive->attributes[j];
        const cgltf_accessor* accessor = attribute.data;

        const uint8_t* attribute_global_buffer = static_cast<const uint8_t*>(accessor->buffer_view->buffer->data);
        size_t primitive_indecies_start_idx = accessor->buffer_view->offset + accessor->offset;
        const float* ptr = reinterpret_cast<const float*>(attribute_global_buffer + primitive_indecies_start_idx);

        if (attribute.type == cgltf_attribute_type_position)
            positions = ptr;
        else if (attribute.type == cgltf_attribute_type_normal)
            normals = ptr;
        else if (attribute.type == cgltf_attribute_type_texcoord)
            textcoords = ptr;
    }
    auto read_index = [&](size_t idx) -> uint32_t
    {
        if (primitive_indices_buffer->component_type == cgltf_component_type_r_16u)
            return reinterpret_cast<const uint16_t*>(index_data)[idx];
        else if (primitive_indices_buffer->component_type == cgltf_component_type_r_32u)
            return reinterpret_cast<const uint32_t*>(index_data)[idx];
        return 0;
    };

    std::unordered_map<uint32_t, uint32_t> map;
    for (size_t j = 0; j < primitive_indices_buffer->count; ++j)
    {
        uint32_t vi = read_index(j);
        if (map.contains(vi))
        {
            indices.push_back(map[vi]);
            continue;
        }
        VertexData v{};

        if (positions)
        {
            v.pos.x = positions[vi * 3 + 0];
            v.pos.y = positions[vi * 3 + 1];
            v.pos.z = positions[vi * 3 + 2];
        }
        if (normals)
        {
            v.normal.x = normals[vi * 3 + 0];
            v.normal.y = normals[vi * 3 + 1];
            v.normal.z = normals[vi * 3 + 2];
        }
        if (textcoords)
        {
            v.textcoord.x = textcoords[vi * 2 + 0];
            v.textcoord.y = textcoords[vi * 2 + 1];
        }

        vertices.push_back(v);
        uint32_t new_index = static_cast<uint32_t>(vertices.size() - 1);
        map[vi] = new_index;
        indices.push_back(new_index);
    }
    primitive_vertex_end = vertices.size() - 1;
    primitive_index_end = indices.size() - 1;
}
