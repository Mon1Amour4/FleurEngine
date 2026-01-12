#pragma once

#include "Graphics.hpp"
// #include "Material.h"

namespace Fleur::Graphics
{
struct SVertexData;
class Material;

class FLEUR_API Model
{
    friend class CGLTFModelFabric;
    friend class MeshBuilder;

public:
    class FLEUR_API Mesh
    {
        friend class CGLTFModelFabric;
        friend class MeshBuilder;

    public:
        class FLEUR_API Primitive
        {
            friend class CGLTFModelFabric;
            friend class MeshBuilder;

        public:
            Primitive();
            ~Primitive() = default;

            inline uint32_t VertexCount() const
            {
                return m_PrimitiveVertexCount;
            }
            inline uint32_t IndexCount() const
            {
                return m_PrimitiveIndicesCount;
            }

            inline uint32_t VertexStart() const
            {
                return m_PrimitiveVertexStart;
            }
            inline uint32_t VertexEnd() const
            {
                return m_PrimitiveVertexEnd;
            }

            inline uint32_t IndexStart() const
            {
                return m_PrimitiveIndexStart;
            }
            inline uint32_t IndexEnd() const
            {
                return m_PrimitiveIndexEnd;
            }

            [[nodiscard]] inline uint32_t VertexSize() const
            {
                return m_PrimitiveVertexCount * sizeof(float);
            }
            [[nodiscard]] inline uint32_t IndexSize() const
            {
                return m_PrimitiveIndicesCount * sizeof(uint32_t);
            }

            [[nodiscard]] inline uint32_t MaterialIdx() const
            {
                return m_MatIdx;
            }

            enum PrimitiveShape
            {
                Quad,
                Sphere,
                Trinagle
            };

        private:
            uint32_t m_MatIdx;

            uint32_t m_PrimitiveVertexStart;
            uint32_t m_PrimitiveVertexEnd;

            uint32_t m_PrimitiveIndexStart;
            uint32_t m_PrimitiveIndexEnd;

            uint32_t m_PrimitiveVertexCount;
            uint32_t m_PrimitiveIndicesCount;
        };

        Mesh();
        ~Mesh() = default;

        Mesh(Mesh&& other) noexcept;

        inline std::string_view Name() const
        {
            return m_MeshName;
        }

        inline const Primitive* Primitives() const
        {
            if (m_Primitives.size() == 0)
                return nullptr;
            return &m_Primitives[0];
        }
        inline uint32_t PrimitivesCount() const
        {
            return static_cast<uint32_t>(m_Primitives.size());
        }

    private:
        std::vector<Primitive> m_Primitives;

        std::string m_MeshName;

        uint32_t m_MeshVertexStart;
        uint32_t m_MeshVertexEnd;
        uint32_t m_MeshIndexStart;
        uint32_t m_MeshIndexEnd;
        uint32_t m_MeshVertexCount;
        uint32_t m_MeshIndicesCount;
    };

    Model() = default;
    Model(std::string_view modelName);

    ~Model() = default;

    Model(Model&& other) noexcept;
    Model& operator=(Model&& other) noexcept;

    inline std::string_view GetName() const
    {
        return m_Name;
    }
    inline uint32_t GetMeshCount() const
    {
        return m_MeshCount;
    }
    inline uint32_t GetVertexCount() const
    {
        return m_ModelVertexCount;
    }
    inline uint32_t GetIndicesCount() const
    {
        return m_ModelIndicesCount;
    }
    inline uint32_t GetMaterialsCount() const
    {
        return m_Materials.size();
    }
    inline uint32_t GetPrimitiveCount() const
    {
        return m_PrimitiveCount;
    }

    [[nodiscard]] inline const Fleur::Graphics::SVertexData* GetVerticesData() const
    {
        return m_Vertices.data();
    }
    [[nodiscard]] inline const uint32_t* GetIndicesData() const
    {
        return m_Indices.data();
    }
    [[nodiscard]] const Fleur::Graphics::Model::Mesh* GetMeshData() const
    {
        return m_Meshes.data();
    }
    [[nodiscard]] inline const Fleur::Graphics::Material* GetMaterialsData() const
    {
        return m_Materials.data();
    }

    const Material* GetMaterial(uint32_t idx) const;

    MeshBuilder CreateSubmesh(std::string_view meshName);
    static Model* QuadModel(Fleur::Graphics::Material&& material);

    struct SFLPostCreateInfo
    {
        std::vector<Fleur::Graphics::Model::Mesh> meshes;
        std::vector<Fleur::Graphics::Material> materials;
        std::vector<Fleur::Graphics::SVertexData> m_Vertices;
        std::vector<uint32_t> m_Indices;
        uint32_t modelVertexCount;
        uint32_t modelIndicesCount;
        uint32_t primitiveCount;
    };
    void PostCreate(SFLPostCreateInfo& info);

private:
    std::string m_Name;
    uint32_t m_MeshCount;
    uint32_t m_PrimitiveCount;
    uint32_t m_ModelVertexCount;
    uint32_t m_ModelIndicesCount;
    std::vector<Fleur::Graphics::SVertexData> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::vector<Model::Mesh> m_Meshes;

    std::vector<Material> m_Materials;
};

class FLEUR_API MeshBuilder
{
public:
    MeshBuilder(std::string_view meshName, Model* model);
    MeshBuilder& AddPrimitive(Model::Mesh::Primitive::PrimitiveShape shape);
    void Commit();

private:
    std::string_view m_Name;
    Model* m_Model;
    Model::Mesh* m_Mesh;
};
}  // namespace Fleur::Graphics
