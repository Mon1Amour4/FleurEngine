#pragma once

#include "External/cgltf/cgltf.h"

namespace Fleur::Graphics
{
class Model;

class ModelFabricBase
{
public:
    ModelFabricBase() = default;
    virtual ~ModelFabricBase() = default;

    void process_model(cgltf_data* data, bool async = true);
    virtual Model* ProcessModel(bool async = true) = 0;
};

class CGLTFModelFabric : public ModelFabricBase
{
public:
    CGLTFModelFabric(std::string_view name, const cgltf_data const* data);
    virtual ~CGLTFModelFabric() override = default;

    virtual Model* ProcessModel(bool async = true) override;

private:
    const cgltf_data* const m_Data;
    std::string_view m_Name;
};
}  // namespace Fleur::Graphics
