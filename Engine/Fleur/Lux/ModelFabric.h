#pragma once

#include "AssetsManager.h"
#include "External/cgltf/cgltf.h"
#include "Model.h"

namespace Fleur::Graphics
{

class ModelFabricBase
{
public:
    ModelFabricBase() = default;
    virtual ~ModelFabricBase() = default;

    // virtual Model* ProcessModel(bool async = true) = 0;
    virtual Model::SFLPostCreateInfo ProcessData(bool async = true) = 0;
};

class CGLTFModelFabric : public ModelFabricBase
{
public:
    CGLTFModelFabric(std::string_view name, const cgltf_data* const data);
    virtual ~CGLTFModelFabric() override = default;

    // virtual Model* ProcessModel(bool async = true) override;
    virtual Model::SFLPostCreateInfo ProcessData(bool async = true) override;

private:
    const cgltf_data* const m_Data;
    std::string_view m_Name;

    Model::Mesh::Primitive process_primitive(std::vector<Fleur::Graphics::SVertexData>& vertices, std::vector<uint32_t>& indices,
                                             cgltf_primitive& cgltfPrimitive, uint32_t maxIdx, FLAlphaMode alphaMode);
    FLAlphaMode process_alpha_mode(cgltf_alpha_mode mode);

    std::string GetImageAssetKey(const cgltf_image& image) const;
    AssetID LoadImageAsset(const cgltf_image& image, Fleur::AssetsManager& assetsManager, bool srgb = true);
    void ProcessNormalTexture(const cgltf_texture_view& normalTexture, Fleur::Graphics::FLMaterial& material, Fleur::AssetsManager& assetsManager);

    void PrintNodeMeshes(const cgltf_data* data);

    static std::string s_SolidImageName;
};

}  // namespace Fleur::Graphics
