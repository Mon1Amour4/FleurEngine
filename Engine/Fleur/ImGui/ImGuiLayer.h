#pragma once

#include "Layer.h"

namespace Fleur
{
class FLEUR_API ImGuiLayer : public Layer
{
public:
    ImGuiLayer(const std::string& name = "ImGui");
    ~ImGuiLayer() = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dtTime) override;
    void OnEvent(EventVariant& event) override;
};
}  // namespace Fleur
