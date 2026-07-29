#pragma once

#include "Lux/Color.h"
#include "Lux/Lux.h"

namespace Fleur::Graphics
{
class IRenderer;
}

namespace Fleur::Graphics
{
class DirectionalLight
{
public:
    DirectionalLight();
    DirectionalLight(Fleur::Math::vec3 direction, Fleur::Graphics::Color color, float intensity);

    inline Fleur::Math::vec3 GetDirection() const
    {
        return m_Direction;
    }
    inline Fleur::Graphics::Color GetColor() const
    {
        return m_Color;
    }
    inline float GetIntensity() const
    {
        return m_Intensity;
    }
    inline Fleur::Math::vec3 GetVirtualPosition() const
    {
        return -m_Direction * s_PosScale;
    }

    void SetDirection(Fleur::Math::vec3);
    void SetColor(Fleur::Graphics::Color color);
    void SetIntensity(float intensity);

    void DebugDraw(Lux::Renderer* renderer, uint32_t textureIdx);
    void DebugDrawToTarget(Lux::Renderer* renderer, const Fleur::Math::vec3& targetCenter, float targetRadius, Fleur::Graphics::Color color);

private:
    static float s_PosScale;

    float m_Intensity;
    // Normalized
    Fleur::Math::vec3 m_Direction;

    Fleur::Graphics::Color m_Color;

    struct DirectionalLightDebugBasis
    {
        Fleur::Math::vec3 dir{};
        Fleur::Math::vec3 right{};
        Fleur::Math::vec3 up{};
        bool valid{false};
    };

    DirectionalLightDebugBasis MakeDirectionalLightBasis(const Fleur::Math::vec3& direction);
    void DrawArrowHead(Lux::Renderer* renderer, const Fleur::Math::vec3& end, const DirectionalLightDebugBasis& basis, float arrowSize, Fleur::Graphics::Color color);
    void DrawDirectionalRay(Lux::Renderer* renderer, const Fleur::Math::vec3& start, const Fleur::Math::vec3& end, const DirectionalLightDebugBasis& basis, float arrowSize,
                            bool drawArrow, Fleur::Graphics::Color color);
    Fleur::Math::vec3 GetGridOffset(const DirectionalLightDebugBasis& basis, int x, int y, float spacing);
};

}  // namespace Fleur::Graphics
