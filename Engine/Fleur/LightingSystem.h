#pragma once

#include <cstdint>
#include <vector>

#include "DirectionalLight.h"
#include "Lux/Graphics.hpp"
#include "OmniLight.h"

namespace Fleur::Graphics
{

using Color = Fleur::Graphics::Color;

enum class LightType : uint8_t
{
    Point,
    Directional,
};

struct LightHandle
{
    uint32_t index{UINT32_MAX};
    int32_t generation{0};
    LightType type{LightType::Point};

    [[nodiscard]] bool IsValid() const
    {
        return index != UINT32_MAX;
    }
};

struct PointLightHandle
{
    LightHandle value{};

    [[nodiscard]] operator LightHandle() const
    {
        return value;
    }
};

struct DirectionalLightHandle
{
    LightHandle value{};

    [[nodiscard]] operator LightHandle() const
    {
        return value;
    }
};

struct LightingFrameData
{
    bool pointLightsDirty{false};
    std::vector<SFLPointLight> pointLights;
    DirectionalLightRenderData directionalLight;
};

class LightingSystem
{
public:
    explicit LightingSystem(uint32_t maxPointLights);

    // Not thread-safe. All methods must be called from the main thread.
    PointLightHandle CreatePointLight(Vec3 position, float radius, Color color, float intensity, bool enabled = true);
    DirectionalLightHandle CreateDirectionalLight(Vec3 direction, Color color, float intensity);

    void Destroy(LightHandle handle);

    void Enable(LightHandle handle);
    void Disable(LightHandle handle);
    [[nodiscard]] bool IsEnabled(LightHandle handle) const;

    void SetPosition(PointLightHandle handle, Vec3 position);
    void SetRadius(PointLightHandle handle, float radius);
    void SetDirection(DirectionalLightHandle handle, Vec3 direction);
    [[nodiscard]] DirectionalLight* GetDirectionalLight(DirectionalLightHandle handle);
    [[nodiscard]] const DirectionalLight* GetDirectionalLight(DirectionalLightHandle handle) const;
    void SetColor(LightHandle handle, Color color);
    void SetIntensity(LightHandle handle, float intensity);

    void Update(float deltaTime);

    [[nodiscard]] LightingFrameData BuildFrameData() const;
    [[nodiscard]] LightingFrameData ConsumeFrameData();
    void ClearPointLightsDirty()
    {
        m_PointLightsDirty = false;
    }
    [[nodiscard]] uint32_t GetMaxPointLights() const
    {
        return m_MaxPointLights;
    }

    void Clear();

private:
    struct PointLightSlot
    {
        Fleur::Graphics::OmniLight light;
        int32_t generation{-1};
        bool enabled{false};
    };

    struct DirectionalLightSlot
    {
        Fleur::Graphics::DirectionalLight light;
        int32_t generation{-1};
        bool enabled{false};
    };

    [[nodiscard]] PointLightSlot* GetPointLightSlot(PointLightHandle handle);
    [[nodiscard]] const PointLightSlot* GetPointLightSlot(PointLightHandle handle) const;
    [[nodiscard]] DirectionalLightSlot* GetDirectionalLightSlot(DirectionalLightHandle handle);
    [[nodiscard]] const DirectionalLightSlot* GetDirectionalLightSlot(DirectionalLightHandle handle) const;

    std::vector<PointLightSlot> m_PointLights;
    std::vector<uint32_t> m_FreePointLightIndices;
    DirectionalLightSlot m_DirectionalLight;
    uint32_t m_MaxPointLights{0};
    bool m_PointLightsDirty{true};
};
}  // namespace Fleur::Graphics
