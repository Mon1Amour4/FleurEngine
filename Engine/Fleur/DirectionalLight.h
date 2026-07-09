#pragma ocne

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
    DirectionalLight(glm::vec3 direction, Fleur::Graphics::Color color, float intensity);

    inline glm::vec3 GetDirection() const
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
    inline glm::vec3 GetVirtualPosition() const
    {
        return -m_Direction * s_PosScale;
    }

    void SetDirection(glm::vec3);

    void DebugDraw(Lux::Renderer* renderer, Fleur::Graphics::Color color);
    void DebugDrawToTarget(Lux::Renderer* renderer, const glm::vec3& targetCenter, float targetRadius, Fleur::Graphics::Color color);

private:
    static float s_PosScale;

    float m_Intensity;
    // Normalized
    glm::vec3 m_Direction;

    Fleur::Graphics::Color m_Color;

    struct DirectionalLightDebugBasis
    {
        glm::vec3 dir{};
        glm::vec3 right{};
        glm::vec3 up{};
        bool valid{false};
    };

    DirectionalLightDebugBasis MakeDirectionalLightBasis(const glm::vec3& direction);
    void DrawArrowHead(Lux::Renderer* renderer, const glm::vec3& end, const DirectionalLightDebugBasis& basis, float arrowSize, Fleur::Graphics::Color color);
    void DrawDirectionalRay(Lux::Renderer* renderer, const glm::vec3& start, const glm::vec3& end, const DirectionalLightDebugBasis& basis, float arrowSize,
                            bool drawArrow, Fleur::Graphics::Color color);
    glm::vec3 GetGridOffset(const DirectionalLightDebugBasis& basis, int x, int y, float spacing);
};

}  // namespace Fleur::Graphics
