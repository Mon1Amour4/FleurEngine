#include "ImGuiLayer.h"

#include "imgui.h"

Fleur::ImGuiLayer::ImGuiLayer(const std::string& name)
    : Layer(name)
{
}

void Fleur::ImGuiLayer::OnAttach()
{
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    auto io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
}

void Fleur::ImGuiLayer::OnDetach()
{
}

void Fleur::ImGuiLayer::OnUpdate(float dtTime)
{
    UNUSED(dtTime);
    // TODO
}

void Fleur::ImGuiLayer::OnEvent(EventVariant& event)
{
    UNUSED(event);
}
