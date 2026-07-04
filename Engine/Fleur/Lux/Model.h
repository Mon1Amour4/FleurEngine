#pragma once

#include "Graphics.hpp"

namespace Fleur::Graphics
{
struct SVertexData;

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
            Primitive() = default;
            ~Primitive() = default;

            inline uint32_t GetVertexCount() const
            {
                return m_VertexCount;
            }
            inline uint32_t GetIdxCount() const
            {
                return m_IdxCount;
            }

            inline uint32_t GetVertexStart() const
            {
                return m_VertexStart;
            }
            inline uint32_t GetVertexEnd() const
            {
                return m_VertexEnd;
            }

            inline uint32_t GetIdxStart() const
            {
                return m_IdxStart;
            }
            inline uint32_t GetIdxEnd() const
            {
                return m_IdxEnd;
            }

            inline uint32_t GetVertexBufferSizeBytes() const
            {
                return m_VertexCount * sizeof(SVertexData);
            }
            inline uint32_t GetIdxBufferSizeBytes() const
            {
                return m_IdxCount * sizeof(uint32_t);
            }

            inline uint32_t GetMaterialIdx() const
            {
                return m_MatIdx;
            }

            inline FLAlphaMode GetAlphaMode() const
            {
                return m_AlphaMode;
            }

            enum PrimitiveShape
            {
                Quad,
                Sphere,
                Trinagle
            };

            inline BoundingBox GetBoundingBox() const
            {
                return m_BoundingBox;
            }

        private:
            BoundingBox m_BoundingBox;

            uint32_t m_MatIdx{0};

            uint32_t m_VertexStart{0};
            uint32_t m_VertexEnd{0};

            uint32_t m_IdxStart{0};
            uint32_t m_IdxEnd{0};

            uint32_t m_VertexCount{0};
            uint32_t m_IdxCount{0};

            FLAlphaMode m_AlphaMode{FLAlphaMode::FL_OPAQUE};
        };

        Mesh() = default;
        ~Mesh() = default;

        Mesh(Mesh&& other) noexcept;

        inline std::string_view GetName() const
        {
            return m_MeshName;
        }

        inline const Primitive* GetPrimitives() const
        {
            if (m_Primitives.size() == 0)
                return nullptr;
            return &m_Primitives[0];
        }
        inline uint32_t GetPrimitiveCount() const
        {
            return static_cast<uint32_t>(m_Primitives.size());
        }

        inline const BoundingBox GetBoundingBox() const
        {
            return m_BoundingBox;
        }

    private:
        BoundingBox m_BoundingBox;

        std::vector<Primitive> m_Primitives;

        std::string m_MeshName;

        uint32_t m_MeshVertexStart{0};
        uint32_t m_MeshVertexEnd{0};
        uint32_t m_MeshIndexStart{0};
        uint32_t m_MeshIndexEnd{0};
        uint32_t m_MeshVertexCount{0};
        uint32_t m_MeshIndicesCount{0};
    };

    struct FLMeshInstance
    {
        uint32_t meshIdx{0};
        uint32_t drawCount{0};
        uint32_t transformStartIdx{0};
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
        return m_Meshes.size();
    }
    inline uint32_t GetVertexCount() const
    {
        return m_Vertices.size();
    }
    inline uint32_t GetIdxCount() const
    {
        return m_Indices.size();
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
    [[nodiscard]] inline const uint32_t* GetIdxData() const
    {
        return m_Indices.data();
    }
    [[nodiscard]] const Fleur::Graphics::Model::Mesh* GetMeshData() const
    {
        return m_Meshes.data();
    }
    [[nodiscard]] inline const Fleur::Graphics::FLMaterial* GetMaterialsData() const
    {
        return m_Materials.data();
    }

    const FLMaterial* GetMaterial(uint32_t idx) const;

    struct SFLPostCreateInfo
    {
        std::vector<Fleur::Graphics::Model::Mesh> meshes;
        std::vector<Fleur::Graphics::FLMaterial> materials;

        std::vector<FLMeshInstance> meshInstance;
        std::vector<glm::mat4> worldTransforms;

        std::vector<Fleur::Graphics::SVertexData> m_Vertices;
        std::vector<uint32_t> m_Indices;
        uint32_t modelVertexCount;
        uint32_t modelIndicesCount;
        uint32_t primitiveCount;

        BoundingBox modelBoundingBox;
    };
    void PostCreate(SFLPostCreateInfo&& info);

    const glm::mat4* GetNodeTransforms() const
    {
        return m_WorldTransforms.data();
    }
    const uint32_t GetNodeTransformsCount() const
    {
        return m_WorldTransforms.size();
    }

    const Model::FLMeshInstance* GetMeshInstanceData() const
    {
        return m_MeshInstance.data();
    }
    const uint32_t GetMeshInstanceCount() const
    {
        return m_MeshInstance.size();
    }
    inline const BoundingBox GetBoundingBox() const
    {
        return m_BoundingBox;
    }

private:
    BoundingBox m_BoundingBox;
    std::string m_Name;
    uint32_t m_PrimitiveCount{0};

    std::vector<Model::Mesh> m_Meshes;
    std::vector<Fleur::Graphics::SVertexData> m_Vertices;
    std::vector<uint32_t> m_Indices;

    std::vector<FLMaterial> m_Materials;

    std::vector<FLMeshInstance> m_MeshInstance;
    std::vector<glm::mat4> m_WorldTransforms;
};
}  // namespace Fleur::Graphics
