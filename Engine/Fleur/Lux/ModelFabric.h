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

    Model::Mesh::Primitive process_primitive(std::vector<Fleur::Graphics::SVertexData>& vertices, std::vector<uint32_t>& indices, SAABB& aabb,
                                             cgltf_primitive& cgltfPrimitive, uint32_t maxIdx, FLAlphaMode alphaMode);
    FLAlphaMode process_alpha_mode(cgltf_alpha_mode mode);
};

}  // namespace Fleur::Graphics
