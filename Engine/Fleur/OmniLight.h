#pragma once

#include "Lux/Color.h"

namespace Lux
{
class Renderer;
}

namespace Fleur::Graphics
{
class OmniLight
{
public:
    OmniLight();
    OmniLight(Fleur::Math::vec3 position, float radius, Fleur::Graphics::Color color, float intensity);

    Fleur::Math::vec3 GetPosition() const
    {
        return m_Pos;
    }
    float GetRadius() const
    {
        return m_Radius;
    }
    Fleur::Graphics::Color GetColor() const
    {
        return m_Color;
    }
    float GetIntensity() const
    {
        return m_Intensity;
    }
    void SetPosition(Fleur::Math::vec3 position);
    void SetRadius(float radius);
    void SetColor(Fleur::Graphics::Color color);
    void SetIntensity(float intensity);
    void DebugDraw(Lux::Renderer* renderer, Fleur::Graphics::Color debugColor) const;
    void DebugDrawToTarget(Lux::Renderer* renderer, const Fleur::Math::vec3& targetCenter, float targetRadius, Fleur::Graphics::Color color) const;

private:
    Fleur::Math::vec3 m_Pos;
    float m_Radius;

    Fleur::Graphics::Color m_Color;
    float m_Intensity;
};
}  // namespace Fleur::Graphics
