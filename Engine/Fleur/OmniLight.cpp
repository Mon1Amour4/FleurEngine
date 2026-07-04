#include "OmniLight.h"

#include "Lux/Lux.h"

Fleur::Graphics::OmniLight::OmniLight(glm::vec3 position, float radius, Fleur::Graphics::Color color, float intensity)
    : m_Pos(position)
    , m_Radius(radius)
    , m_Color(color)
    , m_Intensity(intensity)
{
    assert(radius > 0);
}

void Fleur::Graphics::OmniLight::DebugDraw(Lux::Renderer* renderer, Fleur::Graphics::Color color) const
{
    if (!renderer)
        return;

    if (m_Radius <= 0.0f)
        return;

    constexpr int segments = 48;
    constexpr float twoPi = 6.28318530718f;

    const glm::vec3 xAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 yAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 zAxis = glm::vec3(0.0f, 0.0f, 1.0f);

    // XY circle
    {
        glm::vec3 prev = m_Pos + xAxis * m_Radius;

        for (int i = 1; i <= segments; ++i)
        {
            const float angle = twoPi * static_cast<float>(i) / static_cast<float>(segments);

            const glm::vec3 current = m_Pos + xAxis * std::cos(angle) * m_Radius + yAxis * std::sin(angle) * m_Radius;

            renderer->Debug().Line(prev, current, color);
            prev = current;
        }
    }

    // XZ circle
    {
        glm::vec3 prev = m_Pos + xAxis * m_Radius;

        for (int i = 1; i <= segments; ++i)
        {
            const float angle = twoPi * static_cast<float>(i) / static_cast<float>(segments);

            const glm::vec3 current = m_Pos + xAxis * std::cos(angle) * m_Radius + zAxis * std::sin(angle) * m_Radius;

            renderer->Debug().Line(prev, current, color);
            prev = current;
        }
    }

    // YZ circle
    {
        glm::vec3 prev = m_Pos + yAxis * m_Radius;

        for (int i = 1; i <= segments; ++i)
        {
            const float angle = twoPi * static_cast<float>(i) / static_cast<float>(segments);

            const glm::vec3 current = m_Pos + yAxis * std::cos(angle) * m_Radius + zAxis * std::sin(angle) * m_Radius;

            renderer->Debug().Line(prev, current, color);
            prev = current;
        }
    }

    const float markerSize = m_Radius * 0.05f;

    renderer->Debug().Line(m_Pos - xAxis * markerSize, m_Pos + xAxis * markerSize, color);
    renderer->Debug().Line(m_Pos - yAxis * markerSize, m_Pos + yAxis * markerSize, color);
    renderer->Debug().Line(m_Pos - zAxis * markerSize, m_Pos + zAxis * markerSize, color);
}

void Fleur::Graphics::OmniLight::DebugDrawToTarget(Lux::Renderer* renderer, const glm::vec3& targetCenter, float targetRadius,
                                                   Fleur::Graphics::Color color) const
{
    if (!renderer)
        return;

    if (targetRadius <= 0.0f)
        return;

    const glm::vec3 toTarget = targetCenter - m_Pos;

    const float distanceSq = glm::dot(toTarget, toTarget);
    if (distanceSq <= 0.000001f)
        return;

    const float distance = std::sqrt(distanceSq);
    const glm::vec3 dir = toTarget / distance;

    const glm::vec3 helper = std::abs(dir.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);

    const glm::vec3 right = glm::normalize(glm::cross(helper, dir));
    const glm::vec3 up = glm::normalize(glm::cross(dir, right));

    const int gridHalfSize = 1;

    const float hitRadius = targetRadius * 0.2f;
    const float spacing = hitRadius / static_cast<float>(gridHalfSize);

    const glm::vec3 hitCenter = targetCenter - dir * targetRadius * 0.15f;

    for (int x = -gridHalfSize; x <= gridHalfSize; ++x)
    {
        for (int y = -gridHalfSize; y <= gridHalfSize; ++y)
        {
            const glm::vec3 offset = right * static_cast<float>(x) * spacing + up * static_cast<float>(y) * spacing;

            const glm::vec3 end = hitCenter + offset;

            renderer->Debug().Line(m_Pos, end, color);
        }
    }

    renderer->Debug().Line(m_Pos, hitCenter, color);
}
