#include "OmniLight.h"

#include "Lux/Lux.h"

Fleur::Graphics::OmniLight::OmniLight()
    : m_Pos(0.0f)
    , m_Radius(0.0f)
    , m_Color(Fleur::Graphics::Color::Black())
    , m_Intensity(0.0f)
{
}

Fleur::Graphics::OmniLight::OmniLight(Fleur::Math::vec3 position, float radius, Fleur::Graphics::Color color, float intensity)
    : m_Pos(position)
    , m_Radius(radius)
    , m_Color(color)
    , m_Intensity(intensity)
{
    assert(radius > 0);
}

void Fleur::Graphics::OmniLight::SetPosition(Fleur::Math::vec3 position)
{
    m_Pos = position;
}

void Fleur::Graphics::OmniLight::SetRadius(float radius)
{
    if (radius <= 0.0f)
        return;

    m_Radius = radius;
}

void Fleur::Graphics::OmniLight::SetColor(Fleur::Graphics::Color color)
{
    m_Color = color;
}

void Fleur::Graphics::OmniLight::SetIntensity(float intensity)
{
    m_Intensity = intensity;
}

void Fleur::Graphics::OmniLight::DebugDraw(Lux::Renderer* renderer, Fleur::Graphics::Color color) const
{
    if (!renderer)
        return;

    if (m_Radius <= 0.0f)
        return;

    constexpr int segments = 48;
    constexpr float twoPi = 6.28318530718f;

    const Fleur::Math::vec3 xAxis = Fleur::Math::vec3(1.0f, 0.0f, 0.0f);
    const Fleur::Math::vec3 yAxis = Fleur::Math::vec3(0.0f, 1.0f, 0.0f);
    const Fleur::Math::vec3 zAxis = Fleur::Math::vec3(0.0f, 0.0f, 1.0f);

    // XY circle
    {
        Fleur::Math::vec3 prev = m_Pos + xAxis * m_Radius;

        for (int i = 1; i <= segments; ++i)
        {
            const float angle = twoPi * static_cast<float>(i) / static_cast<float>(segments);

            const Fleur::Math::vec3 current = m_Pos + xAxis * std::cos(angle) * m_Radius + yAxis * std::sin(angle) * m_Radius;

            renderer->Debug().Line(prev, current, color);
            prev = current;
        }
    }

    // XZ circle
    {
        Fleur::Math::vec3 prev = m_Pos + xAxis * m_Radius;

        for (int i = 1; i <= segments; ++i)
        {
            const float angle = twoPi * static_cast<float>(i) / static_cast<float>(segments);

            const Fleur::Math::vec3 current = m_Pos + xAxis * std::cos(angle) * m_Radius + zAxis * std::sin(angle) * m_Radius;

            renderer->Debug().Line(prev, current, color);
            prev = current;
        }
    }

    // YZ circle
    {
        Fleur::Math::vec3 prev = m_Pos + yAxis * m_Radius;

        for (int i = 1; i <= segments; ++i)
        {
            const float angle = twoPi * static_cast<float>(i) / static_cast<float>(segments);

            const Fleur::Math::vec3 current = m_Pos + yAxis * std::cos(angle) * m_Radius + zAxis * std::sin(angle) * m_Radius;

            renderer->Debug().Line(prev, current, color);
            prev = current;
        }
    }

    const float markerSize = m_Radius * 0.05f;

    renderer->Debug().Line(m_Pos - xAxis * markerSize, m_Pos + xAxis * markerSize, color);
    renderer->Debug().Line(m_Pos - yAxis * markerSize, m_Pos + yAxis * markerSize, color);
    renderer->Debug().Line(m_Pos - zAxis * markerSize, m_Pos + zAxis * markerSize, color);
}

void Fleur::Graphics::OmniLight::DebugDrawToTarget(Lux::Renderer* renderer, const Fleur::Math::vec3& targetCenter, float targetRadius,
                                                   Fleur::Graphics::Color color) const
{
    if (!renderer)
        return;

    if (targetRadius <= 0.0f)
        return;

    const Fleur::Math::vec3 toTarget = targetCenter - m_Pos;

    const float distanceSq = Fleur::Math::dot(toTarget, toTarget);
    if (distanceSq <= 0.000001f)
        return;

    const float distance = std::sqrt(distanceSq);
    const Fleur::Math::vec3 dir = toTarget / distance;

    const Fleur::Math::vec3 helper = std::abs(dir.y) < 0.99f ? Fleur::Math::vec3(0.0f, 1.0f, 0.0f) : Fleur::Math::vec3(1.0f, 0.0f, 0.0f);

    const Fleur::Math::vec3 right = Fleur::Math::normalize(Fleur::Math::cross(helper, dir));
    const Fleur::Math::vec3 up = Fleur::Math::normalize(Fleur::Math::cross(dir, right));

    const int gridHalfSize = 1;

    const float hitRadius = targetRadius * 0.2f;
    const float spacing = hitRadius / static_cast<float>(gridHalfSize);

    const Fleur::Math::vec3 hitCenter = targetCenter - dir * targetRadius * 0.15f;

    for (int x = -gridHalfSize; x <= gridHalfSize; ++x)
    {
        for (int y = -gridHalfSize; y <= gridHalfSize; ++y)
        {
            const Fleur::Math::vec3 offset = right * static_cast<float>(x) * spacing + up * static_cast<float>(y) * spacing;

            const Fleur::Math::vec3 end = hitCenter + offset;

            renderer->Debug().Line(m_Pos, end, color);
        }
    }

    renderer->Debug().Line(m_Pos, hitCenter, color);
}
