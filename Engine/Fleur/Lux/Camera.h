#pragma once

#include "Services/ServiceInterfaces.hpp"

using vec3 = glm::vec3;
using mat4 = glm::mat4;

namespace Fleur::Graphics
{
class FLEUR_API Camera : IUpdatable
{
public:
    Camera();
    ~Camera() = default;

    FLEUR_NON_COPYABLE_NON_MOVABLE(Camera);

    inline mat4 GetView() const
    {
        return m_View;
    }
    float Yaw() const
    {
        return m_Yaw;
    }
    float Pitch() const
    {
        return m_Pitch;
    }
    inline mat4 GetProjection() const
    {
        return m_Projection;
    }
    inline vec3 GetCameraForward() const
    {
        return m_CameraForward;
    }

    const mat4* GetViewPtr() const;

    static Camera* GetActiveCamera();

    void Activate();

    void OnUpdate(float dtTime);
    void OnPostUpdate(float dtTime);
    void OnFixedUpdate();

    float FarClip() const;
    float NearClip() const;

private:
    float m_Speed{50.f};
    void UpdateForward();
    void RotateCamera(float dtTime);
    static Camera* s_ActiveCamera;
    vec3 m_Position{0.f};
    mat4 m_View;
    mat4 m_Projection;

    vec3 m_CameraForward;
    vec3 m_Up;

    uint16_t m_FOV{60};

    float m_NearClip{0.1f};
    float m_FarClip{3000.f};

    float m_Yaw{0.f};
    float m_Pitch{0.f};
    // float m_MouseSensitivity{10.f};
    float m_MouseSensitivity{0.2f};
};
}  // namespace Fleur::Graphics
