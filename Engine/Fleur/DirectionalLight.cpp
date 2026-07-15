#include "DirectionalLight.h"

#include <cstdlib>


float Fleur::Graphics::DirectionalLight::s_PosScale = 100;

Fleur::Graphics::DirectionalLight::DirectionalLight(glm::vec3 direction, Fleur::Graphics::Color color, float intensity)
    : m_Direction(glm::normalize(direction))
    , m_Color(color)
    , m_Intensity(intensity)
{
}

void Fleur::Graphics::DirectionalLight::SetDirection(glm::vec3 dir)
{
    if (dir == glm::vec3(0, 0, 0))
        return;

    m_Direction = glm::normalize(dir);
}

void Fleur::Graphics::DirectionalLight::DebugDraw(Lux::Renderer* renderer, uint32_t textureIdx)
{
    if (!renderer)
        return;

    float radius = 10.0f;
    glm::vec3 pos = GetVirtualPosition();
    glm::vec3 forward = glm::normalize(m_Direction);

    glm::vec3 upHint(0, 1, 0);
    if (std::abs(glm::dot(forward, upHint)) > 0.99f)
        upHint = glm::vec3(1, 0, 0);

    glm::vec3 right = glm::normalize(glm::cross(upHint, forward));
    glm::vec3 up = glm::normalize(glm::cross(forward, right));

    glm::mat3 basis(right, up, forward);

    glm::vec3 localA(-radius, radius, 0.0f);
    glm::vec3 localB(radius, radius, 0.0f);
    glm::vec3 localC(radius, -radius, 0.0f);
    glm::vec3 localD(-radius, -radius, 0.0f);

    glm::vec3 A = pos + basis * localA;
    glm::vec3 B = pos + basis * localB;
    glm::vec3 C = pos + basis * localC;
    glm::vec3 D = pos + basis * localD;

    // renderer->Debug().Quad(A, B, C, D, textureIdx);
    renderer->Debug().Line(pos, glm::vec3(0, 0, 0), Fleur::Graphics::Color::Red());
}

void Fleur::Graphics::DirectionalLight::DebugDrawToTarget(Lux::Renderer* renderer, const glm::vec3& targetCenter, float targetRadius,
                                                          Fleur::Graphics::Color color)
{
    if (!renderer)
        return;

    if (targetRadius <= 0.0f)
        return;

    const DirectionalLightDebugBasis basis = MakeDirectionalLightBasis(m_Direction);
    if (!basis.valid)
        return;

    const int gridHalfSize = 2;

    const float spacing = targetRadius * 0.35f;
    const float rayLength = targetRadius * 1.8f;
    const float arrowSize = targetRadius * 0.08f;

    const float radiusSq = targetRadius * targetRadius;

    for (int x = -gridHalfSize; x <= gridHalfSize; ++x)
    {
        for (int y = -gridHalfSize; y <= gridHalfSize; ++y)
        {
            const glm::vec3 offset = GetGridOffset(basis, x, y, spacing);

            const float offsetSq = glm::dot(offset, offset);
            if (offsetSq > radiusSq)
                continue;

            const float capDepth = std::sqrt(std::max(0.0f, radiusSq - offsetSq));

            const glm::vec3 end = targetCenter - basis.dir * capDepth + offset;
            const glm::vec3 start = end - basis.dir * rayLength;

            const bool isCenterRay = x == 0 && y == 0;

            DrawDirectionalRay(renderer, start, end, basis, arrowSize, isCenterRay, color);
        }
    }
}

Fleur::Graphics::DirectionalLight::DirectionalLightDebugBasis Fleur::Graphics::DirectionalLight::MakeDirectionalLightBasis(const glm::vec3& direction)
{
    const float lenSq = glm::dot(direction, direction);
    if (lenSq <= 0.000001f)
        return {};

    DirectionalLightDebugBasis basis{};
    basis.dir = glm::normalize(direction);

    const glm::vec3 helper = std::abs(basis.dir.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);

    basis.right = glm::normalize(glm::cross(helper, basis.dir));
    basis.up = glm::normalize(glm::cross(basis.dir, basis.right));
    basis.valid = true;

    return basis;
}

void Fleur::Graphics::DirectionalLight::DrawArrowHead(Lux::Renderer* renderer, const glm::vec3& end, const DirectionalLightDebugBasis& basis, float arrowSize,
                                                      Fleur::Graphics::Color color)
{
    const glm::vec3 arrowA = end - basis.dir * arrowSize + basis.right * arrowSize * 0.5f;
    const glm::vec3 arrowB = end - basis.dir * arrowSize - basis.right * arrowSize * 0.5f;
    const glm::vec3 arrowC = end - basis.dir * arrowSize + basis.up * arrowSize * 0.5f;
    const glm::vec3 arrowD = end - basis.dir * arrowSize - basis.up * arrowSize * 0.5f;

    renderer->Debug().Line(end, arrowA, color);
    renderer->Debug().Line(end, arrowB, color);
    renderer->Debug().Line(end, arrowC, color);
    renderer->Debug().Line(end, arrowD, color);
}

void Fleur::Graphics::DirectionalLight::DrawDirectionalRay(Lux::Renderer* renderer, const glm::vec3& start, const glm::vec3& end,
                                                           const DirectionalLightDebugBasis& basis, float arrowSize, bool drawArrow,
                                                           Fleur::Graphics::Color color)
{
    renderer->Debug().Line(start, end, color);

    if (drawArrow && arrowSize > 0.0f)
        DrawArrowHead(renderer, end, basis, arrowSize, color);
}

glm::vec3 Fleur::Graphics::DirectionalLight::GetGridOffset(const DirectionalLightDebugBasis& basis, int x, int y, float spacing)
{
    return basis.right * static_cast<float>(x) * spacing + basis.up * static_cast<float>(y) * spacing;
}
