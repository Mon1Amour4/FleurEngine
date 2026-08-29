#include "Fleur/Lux/ModelFabric.h"

#include "Fleur/Services/ServiceLocator.h"
#include "gtest/gtest.h"
#include "Fleur/Math/Functions.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

namespace
{

struct ModelFixture
{
    std::vector<uint8_t> bytes = std::vector<uint8_t>(256, uint8_t{0});

    cgltf_buffer buffer{};
    cgltf_buffer_view interleavedView{};
    cgltf_buffer_view indexView{};
    cgltf_buffer_view sparseIndexView{};
    cgltf_buffer_view sparseValueView{};

    cgltf_accessor positionAccessor{};
    cgltf_accessor normalAccessor{};
    cgltf_accessor texcoordAccessor{};
    cgltf_accessor tangentAccessor{};
    cgltf_accessor indexAccessor{};

    cgltf_attribute attributes[4]{};
    cgltf_primitive primitive{};
    cgltf_mesh mesh{};
    cgltf_material material{};
    cgltf_node node{};
    cgltf_data data{};

    ModelFixture(bool indexed = true, cgltf_primitive_type primitiveType = cgltf_primitive_type_triangles)
    {
        buffer.data = bytes.data();
        buffer.size = bytes.size();

        interleavedView.buffer = &buffer;
        interleavedView.offset = 0;
        interleavedView.size = 148;

        indexView.buffer = &buffer;
        indexView.offset = 160;
        indexView.size = 6;

        sparseIndexView.buffer = &buffer;
        sparseIndexView.offset = 176;
        sparseIndexView.size = 1;

        sparseValueView.buffer = &buffer;
        sparseValueView.offset = 192;
        sparseValueView.size = 12;

        positionAccessor = MakeAccessor(&interleavedView, cgltf_type_vec3, cgltf_component_type_r_32f, 3, 4, 48);
        normalAccessor = MakeAccessor(&interleavedView, cgltf_type_vec3, cgltf_component_type_r_32f, 3, 16, 48);
        normalAccessor.is_sparse = 1;
        normalAccessor.sparse.count = 1;
        normalAccessor.sparse.indices_buffer_view = &sparseIndexView;
        normalAccessor.sparse.indices_component_type = cgltf_component_type_r_8u;
        normalAccessor.sparse.values_buffer_view = &sparseValueView;

        texcoordAccessor = MakeAccessor(&interleavedView, cgltf_type_vec2, cgltf_component_type_r_8u, 3, 28, 48);
        texcoordAccessor.normalized = 1;

        tangentAccessor = MakeAccessor(&interleavedView, cgltf_type_vec4, cgltf_component_type_r_32f, 3, 36, 48);

        attributes[0] = {nullptr, cgltf_attribute_type_position, 0, &positionAccessor};
        attributes[1] = {nullptr, cgltf_attribute_type_normal, 0, &normalAccessor};
        attributes[2] = {nullptr, cgltf_attribute_type_texcoord, 0, &texcoordAccessor};
        attributes[3] = {nullptr, cgltf_attribute_type_tangent, 0, &tangentAccessor};

        primitive.type = primitiveType;
        primitive.indices = indexed ? &indexAccessor : nullptr;
        primitive.material = &material;
        primitive.attributes = attributes;
        primitive.attributes_count = std::size(attributes);

        if (indexed)
        {
            indexAccessor = MakeAccessor(&indexView, cgltf_type_scalar, cgltf_component_type_r_16u, 3, 0, 2);
        }

        mesh.primitives = &primitive;
        mesh.primitives_count = 1;
        mesh.name = const_cast<char*>("fixture");

        material.alpha_mode = cgltf_alpha_mode_opaque;
        material.pbr_metallic_roughness.base_color_factor[0] = 1.0f;
        material.pbr_metallic_roughness.base_color_factor[1] = 1.0f;
        material.pbr_metallic_roughness.base_color_factor[2] = 1.0f;
        material.pbr_metallic_roughness.base_color_factor[3] = 1.0f;

        node.mesh = &mesh;
        node.has_matrix = 1;
        node.matrix[0] = 1.0f;
        node.matrix[5] = 1.0f;
        node.matrix[10] = 1.0f;
        node.matrix[15] = 1.0f;

        data.buffers = &buffer;
        data.buffers_count = 1;
        data.meshes = &mesh;
        data.meshes_count = 1;
        data.materials = &material;
        data.materials_count = 1;
        data.nodes = &node;
        data.nodes_count = 1;

        WriteVec3(4, {10.0f, 20.0f, 30.0f});
        WriteVec3(52, {40.0f, 50.0f, 60.0f});
        WriteVec3(100, {70.0f, 80.0f, 90.0f});

        WriteVec3(16, {0.0f, 0.0f, 1.0f});
        WriteVec3(64, {0.0f, 0.0f, 1.0f});
        WriteVec3(112, {0.0f, 0.0f, 1.0f});

        bytes[28] = 0;
        bytes[29] = 0;
        bytes[76] = 255;
        bytes[77] = 0;
        bytes[124] = 0;
        bytes[125] = 255;

        WriteVec4(36, {1.0f, 0.0f, 0.0f, 1.0f});
        WriteVec4(84, {0.0f, 1.0f, 0.0f, -1.0f});
        WriteVec4(132, {0.0f, 0.0f, 1.0f, 1.0f});

        bytes[176] = 1;
        WriteVec3(192, {1.0f, 0.0f, 0.0f});

        const uint16_t indices[3] = {0, 1, 2};
        std::memcpy(bytes.data() + 160, indices, sizeof(indices));
    }

    void UsePlanarTriangle()
    {
        positionAccessor.count = 3;
        WriteVec3(4, {0.0f, 0.0f, 0.0f});
        WriteVec3(52, {1.0f, 0.0f, 0.0f});
        WriteVec3(100, {0.0f, 1.0f, 0.0f});

        normalAccessor.is_sparse = 0;
        WriteVec3(16, {0.0f, 0.0f, 1.0f});
        WriteVec3(64, {0.0f, 0.0f, 1.0f});
        WriteVec3(112, {0.0f, 0.0f, 1.0f});
    }

    void RemoveTangents()
    {
        attributes[3].data = nullptr;
    }

    void RemoveTexcoords()
    {
        attributes[2].data = nullptr;
    }

    void MakeMirroredTexcoords()
    {
        bytes[28] = 0;
        bytes[29] = 0;
        bytes[76] = 0;
        bytes[77] = 255;
        bytes[124] = 255;
        bytes[125] = 0;
    }

    void MakeInvalidNormals()
    {
        normalAccessor.is_sparse = 0;
        const float nan = std::numeric_limits<float>::quiet_NaN();
        WriteVec3(16, {nan, 0.0f, 1.0f});
        WriteVec3(64, {0.0f, 0.0f, 1.0f});
        WriteVec3(112, {0.0f, 0.0f, 1.0f});
    }

    void RemoveNormals()
    {
        attributes[1].data = nullptr;
    }

private:
    static cgltf_accessor MakeAccessor(cgltf_buffer_view* view, cgltf_type type, cgltf_component_type componentType, cgltf_size count,
                                       cgltf_size offset, cgltf_size stride)
    {
        cgltf_accessor accessor{};
        accessor.buffer_view = view;
        accessor.type = type;
        accessor.component_type = componentType;
        accessor.count = count;
        accessor.offset = offset;
        accessor.stride = stride;
        return accessor;
    }

    void WriteVec3(size_t offset, std::array<float, 3> values)
    {
        std::memcpy(bytes.data() + offset, values.data(), sizeof(values));
    }

    void WriteVec4(size_t offset, std::array<float, 4> values)
    {
        std::memcpy(bytes.data() + offset, values.data(), sizeof(values));
    }
};

Fleur::Graphics::Model::SFLPostCreateInfo Import(ModelFixture& fixture)
{
    auto assets = Fleur::ServiceLocator::instance().Register<Fleur::AssetsManager>();
    EXPECT_TRUE(assets.has_value());
    Fleur::Graphics::CGLTFModelFabric fabric("model.gltf", &fixture.data);
    return fabric.ProcessData(false);
}

TEST(ModelFabricTest, ReadsInterleavedOffsetNormalizedAndSparseAttributes)
{
    ModelFixture fixture;
    auto info = Import(fixture);

    ASSERT_EQ(info.m_Vertices.size(), 3u);
    EXPECT_FLOAT_EQ(info.m_Vertices[0].Position.x, 10.0f);
    EXPECT_FLOAT_EQ(info.m_Vertices[1].Position.x, 40.0f);
    EXPECT_FLOAT_EQ(info.m_Vertices[2].Position.x, 70.0f);

    EXPECT_FLOAT_EQ(info.m_Vertices[0].Normal.z, 1.0f);
    EXPECT_FLOAT_EQ(info.m_Vertices[1].Normal.x, 1.0f);
    EXPECT_FLOAT_EQ(info.m_Vertices[2].Normal.z, 1.0f);

    EXPECT_FLOAT_EQ(info.m_Vertices[0].TexCoord.x, 0.0f);
    EXPECT_FLOAT_EQ(info.m_Vertices[1].TexCoord.x, 1.0f);
    EXPECT_FLOAT_EQ(info.m_Vertices[2].TexCoord.y, 1.0f);

    EXPECT_FLOAT_EQ(info.m_Vertices[0].Tangent.x, 1.0f);
    EXPECT_FLOAT_EQ(info.m_Vertices[1].Tangent.y, 1.0f);
    EXPECT_FLOAT_EQ(info.m_Vertices[1].Tangent.w, -1.0f);
    EXPECT_FLOAT_EQ(info.m_Vertices[2].Tangent.x, 1.0f);
    EXPECT_FLOAT_EQ(info.m_Vertices[2].Tangent.y, 0.0f);
    EXPECT_FLOAT_EQ(info.m_Vertices[2].Tangent.z, 0.0f);
    EXPECT_FLOAT_EQ(info.m_Vertices[2].Tangent.w, 1.0f);
}

TEST(ModelFabricTest, GeneratesFallbackTangentsWhenTangentAttributeIsMissing)
{
    ModelFixture fixture;
    fixture.UsePlanarTriangle();
    fixture.RemoveTangents();
    auto info = Import(fixture);

    ASSERT_EQ(info.m_Vertices.size(), 3u);
    for (const auto& vertex : info.m_Vertices)
    {
        EXPECT_NEAR(vertex.Tangent.x, 1.0f, 1e-5f);
        EXPECT_NEAR(vertex.Tangent.y, 0.0f, 1e-5f);
        EXPECT_NEAR(vertex.Tangent.z, 0.0f, 1e-5f);
        EXPECT_FLOAT_EQ(vertex.Tangent.w, 1.0f);
        EXPECT_NEAR(Fleur::Math::dot(vertex.Normal, Fleur::Vec3(vertex.Tangent)), 0.0f, 1e-5f);
    }
}

TEST(ModelFabricTest, UsesPositiveHandednessForFallbackTangentsWithoutUVs)
{
    ModelFixture fixture;
    fixture.UsePlanarTriangle();
    fixture.RemoveTangents();
    fixture.RemoveTexcoords();
    auto info = Import(fixture);

    ASSERT_EQ(info.m_Vertices.size(), 3u);
    for (const auto& vertex : info.m_Vertices)
    {
        EXPECT_TRUE(std::isfinite(vertex.Tangent.x));
        EXPECT_TRUE(std::isfinite(vertex.Tangent.y));
        EXPECT_TRUE(std::isfinite(vertex.Tangent.z));
        EXPECT_FLOAT_EQ(vertex.Tangent.w, 1.0f);
    }
}

TEST(ModelFabricTest, PreservesMirroredUvHandednessForGeneratedTangents)
{
    ModelFixture fixture;
    fixture.UsePlanarTriangle();
    fixture.RemoveTangents();
    fixture.MakeMirroredTexcoords();
    auto info = Import(fixture);

    ASSERT_EQ(info.m_Vertices.size(), 3u);
    for (const auto& vertex : info.m_Vertices)
        EXPECT_FLOAT_EQ(vertex.Tangent.w, -1.0f);
}

TEST(ModelFabricTest, GeneratesNormalsWhenNormalAttributeIsMissing)
{
    ModelFixture fixture;
    fixture.UsePlanarTriangle();
    fixture.RemoveNormals();
    fixture.RemoveTangents();
    auto info = Import(fixture);

    ASSERT_EQ(info.m_Vertices.size(), 3u);
    for (const auto& vertex : info.m_Vertices)
    {
        EXPECT_NEAR(vertex.Normal.x, 0.0f, 1e-5f);
        EXPECT_NEAR(vertex.Normal.y, 0.0f, 1e-5f);
        EXPECT_NEAR(vertex.Normal.z, 1.0f, 1e-5f);
    }
}

TEST(ModelFabricTest, RejectsPrimitiveWithInvalidNormals)
{
    ModelFixture fixture;
    fixture.UsePlanarTriangle();
    fixture.MakeInvalidNormals();
    auto info = Import(fixture);

    EXPECT_TRUE(info.m_Vertices.empty());
    EXPECT_TRUE(info.m_Indices.empty());
}

TEST(ModelFabricTest, RejectsUnsupportedAttributeAccessorComponentType)
{
    ModelFixture fixture;
    fixture.tangentAccessor.component_type = cgltf_component_type_invalid;
    auto info = Import(fixture);

    EXPECT_TRUE(info.m_Vertices.empty());
    EXPECT_TRUE(info.m_Indices.empty());
}

TEST(ModelFabricTest, CreatesSequentialIndicesForNonIndexedTriangles)
{
    ModelFixture fixture(false);
    auto info = Import(fixture);

    ASSERT_EQ(info.m_Indices.size(), 3u);
    EXPECT_EQ(info.m_Indices[0], 0u);
    EXPECT_EQ(info.m_Indices[1], 1u);
    EXPECT_EQ(info.m_Indices[2], 2u);
}

TEST(ModelFabricTest, RejectsUnsupportedPrimitiveModes)
{
    ModelFixture fixture(true, cgltf_primitive_type_triangle_strip);
    auto info = Import(fixture);

    EXPECT_TRUE(info.m_Vertices.empty());
    EXPECT_TRUE(info.m_Indices.empty());
}

} // namespace
