#include "LightingSystem.h"

#include <algorithm>

namespace Fleur::Graphics
{
LightingSystem::LightingSystem(uint32_t maxPointLights)
    : m_MaxPointLights(maxPointLights)
{
}

PointLightHandle LightingSystem::CreatePointLight(Fleur::Vec3 position, float radius, Fleur::Graphics::Color color, float intensity, bool enabled)
{
    if (m_FreePointLightIndices.empty() && m_PointLights.size() >= m_MaxPointLights)
        return {};

    uint32_t index = 0;
    if (m_FreePointLightIndices.empty())
    {
        index = static_cast<uint32_t>(m_PointLights.size());
        m_PointLights.emplace_back();
    }
    else
    {
        index = m_FreePointLightIndices.back();
        m_FreePointLightIndices.pop_back();
    }

    PointLightSlot& slot = m_PointLights[index];
    slot.light = Fleur::Graphics::OmniLight(position, radius, color, intensity);
    if (slot.generation < 0)
        slot.generation = -slot.generation + 1;
    slot.enabled = enabled;
    m_PointLightsDirty = true;

    return PointLightHandle{LightHandle{index, slot.generation, LightType::Point}};
}

DirectionalLightHandle LightingSystem::CreateDirectionalLight(Fleur::Vec3 direction, Fleur::Graphics::Color color, float intensity)
{
    if (m_DirectionalLight.generation > 0)
        return {};

    DirectionalLightSlot& slot = m_DirectionalLight;
    slot.light = Fleur::Graphics::DirectionalLight(direction, color, intensity);
    if (slot.generation < 0)
        slot.generation = -slot.generation + 1;
    slot.enabled = true;

    return DirectionalLightHandle{LightHandle{0, slot.generation, LightType::Directional}};
}

void LightingSystem::Destroy(LightHandle handle)
{
    if (handle.type == LightType::Point)
    {
        PointLightSlot* slot = GetPointLightSlot(PointLightHandle{handle});
        if (!slot)
            return;

        slot->enabled = false;
        slot->generation = -slot->generation;
        m_FreePointLightIndices.push_back(handle.index);
        m_PointLightsDirty = true;
        return;
    }

    DirectionalLightSlot* slot = GetDirectionalLightSlot(DirectionalLightHandle{handle});
    if (!slot)
        return;

    slot->enabled = false;
    slot->generation = -slot->generation;
}

void LightingSystem::Enable(LightHandle handle)
{
    if (handle.type == LightType::Point)
    {
        if (PointLightSlot* slot = GetPointLightSlot(PointLightHandle{handle}))
        {
            slot->enabled = true;
            m_PointLightsDirty = true;
        }
        return;
    }

    if (DirectionalLightSlot* slot = GetDirectionalLightSlot(DirectionalLightHandle{handle}))
        slot->enabled = true;
}

void LightingSystem::Disable(LightHandle handle)
{
    if (handle.type == LightType::Point)
    {
        if (PointLightSlot* slot = GetPointLightSlot(PointLightHandle{handle}))
        {
            slot->enabled = false;
            m_PointLightsDirty = true;
        }
        return;
    }

    if (DirectionalLightSlot* slot = GetDirectionalLightSlot(DirectionalLightHandle{handle}))
        slot->enabled = false;
}

bool LightingSystem::IsEnabled(LightHandle handle) const
{
    if (handle.type == LightType::Point)
    {
        const PointLightSlot* slot = GetPointLightSlot(PointLightHandle{handle});
        return slot != nullptr && slot->enabled;
    }

    const DirectionalLightSlot* slot = GetDirectionalLightSlot(DirectionalLightHandle{handle});
    return slot != nullptr && slot->enabled;
}

void LightingSystem::SetPosition(PointLightHandle handle, Fleur::Vec3 position)
{
    if (PointLightSlot* slot = GetPointLightSlot(handle))
    {
        slot->light.SetPosition(position);
        m_PointLightsDirty = true;
    }
}

void LightingSystem::SetRadius(PointLightHandle handle, float radius)
{
    if (PointLightSlot* slot = GetPointLightSlot(handle))
    {
        slot->light.SetRadius(radius);
        m_PointLightsDirty = true;
    }
}

void LightingSystem::SetDirection(DirectionalLightHandle handle, Fleur::Vec3 direction)
{
    if (DirectionalLightSlot* slot = GetDirectionalLightSlot(handle))
        slot->light.SetDirection(direction);
}

DirectionalLight* LightingSystem::GetDirectionalLight(DirectionalLightHandle handle)
{
    if (DirectionalLightSlot* slot = GetDirectionalLightSlot(handle))
        return &slot->light;

    return nullptr;
}

const DirectionalLight* LightingSystem::GetDirectionalLight(DirectionalLightHandle handle) const
{
    if (const DirectionalLightSlot* slot = GetDirectionalLightSlot(handle))
        return &slot->light;

    return nullptr;
}

void LightingSystem::SetColor(LightHandle handle, Fleur::Graphics::Color color)
{
    if (handle.type == LightType::Point)
    {
        if (PointLightSlot* slot = GetPointLightSlot(PointLightHandle{handle}))
        {
            slot->light.SetColor(color);
            m_PointLightsDirty = true;
        }
        return;
    }

    if (DirectionalLightSlot* slot = GetDirectionalLightSlot(DirectionalLightHandle{handle}))
        slot->light.SetColor(color);
}

void LightingSystem::SetIntensity(LightHandle handle, float intensity)
{
    if (handle.type == LightType::Point)
    {
        if (PointLightSlot* slot = GetPointLightSlot(PointLightHandle{handle}))
        {
            slot->light.SetIntensity(intensity);
            m_PointLightsDirty = true;
        }
        return;
    }

    if (DirectionalLightSlot* slot = GetDirectionalLightSlot(DirectionalLightHandle{handle}))
        slot->light.SetIntensity(intensity);
}

void LightingSystem::Update(float deltaTime)
{
    (void)deltaTime;
}

LightingFrameData LightingSystem::BuildFrameData() const
{
    LightingFrameData frameData;
    frameData.pointLightsDirty = m_PointLightsDirty;
    frameData.pointLights.reserve(m_PointLights.size());

    for (const PointLightSlot& slot : m_PointLights)
    {
        if (!slot.enabled || slot.generation <= 0)
            continue;

        frameData.pointLights.push_back({slot.light.GetPosition(), slot.light.GetRadius(), slot.light.GetColor().ToVec3(), slot.light.GetIntensity()});
    }

    const DirectionalLightSlot& slot = m_DirectionalLight;
    if (slot.enabled && slot.generation > 0)
    {
        frameData.directionalLight = {
            .dirIntens = Fleur::Vec4(slot.light.GetDirection(), slot.light.GetIntensity()),
            .color = slot.light.GetColor().ToVec4(),
            .pos = Fleur::Vec4(slot.light.GetVirtualPosition(), 1.0f),
        };
    }

    return frameData;
}

LightingFrameData LightingSystem::ConsumeFrameData()
{
    LightingFrameData frameData = BuildFrameData();
    m_PointLightsDirty = false;
    return frameData;
}

void LightingSystem::Clear()
{
    m_PointLights.clear();
    m_FreePointLightIndices.clear();
    m_DirectionalLight = {};
    m_PointLightsDirty = true;
}

LightingSystem::PointLightSlot* LightingSystem::GetPointLightSlot(PointLightHandle handle)
{
    if (handle.value.type != LightType::Point || handle.value.index >= m_PointLights.size())
        return nullptr;

    PointLightSlot& slot = m_PointLights[handle.value.index];
    if (slot.generation <= 0 || slot.generation != handle.value.generation)
        return nullptr;

    return &slot;
}

const LightingSystem::PointLightSlot* LightingSystem::GetPointLightSlot(PointLightHandle handle) const
{
    if (handle.value.type != LightType::Point || handle.value.index >= m_PointLights.size())
        return nullptr;

    const PointLightSlot& slot = m_PointLights[handle.value.index];
    if (slot.generation <= 0 || slot.generation != handle.value.generation)
        return nullptr;

    return &slot;
}

LightingSystem::DirectionalLightSlot* LightingSystem::GetDirectionalLightSlot(DirectionalLightHandle handle)
{
    if (handle.value.type != LightType::Directional || handle.value.index != 0)
        return nullptr;

    DirectionalLightSlot& slot = m_DirectionalLight;
    if (slot.generation <= 0 || slot.generation != handle.value.generation)
        return nullptr;

    return &slot;
}

const LightingSystem::DirectionalLightSlot* LightingSystem::GetDirectionalLightSlot(DirectionalLightHandle handle) const
{
    if (handle.value.type != LightType::Directional || handle.value.index != 0)
        return nullptr;

    const DirectionalLightSlot& slot = m_DirectionalLight;
    if (slot.generation <= 0 || slot.generation != handle.value.generation)
        return nullptr;

    return &slot;
}
}  // namespace Fleur::Graphics
