#pragma once

#include "Services/ServiceInterfaces.hpp"

using Vec3 = Fleur::Vec3;
using Mat4 = Fleur::Mat4;

namespace Fleur::Graphics
{
class FLEUR_API Camera : IUpdatable
{
public:
    Camera();
    ~Camera() = default;

    FLEUR_NON_COPYABLE_NON_MOVABLE(Camera);

    inline Mat4 GetView() const
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
    inline Mat4 GetProjection() const
    {
        return m_Projection;
    }
    inline Vec3 GetCameraForward() const
    {
        return m_CameraForward;
    }
    inline Vec3 GetPosition() const
    {
        return m_Position;
    }

    const Mat4* GetViewPtr() const;

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
    Vec3 m_Position{0.f};
    Mat4 m_View{Fleur::Mat4(1.0f)};
    Mat4 m_Projection{Fleur::Mat4(1.0f)};

    Vec3 m_CameraForward{Fleur::Vec3(0.0f, 0.0f, -1.0f)};
    Vec3 m_Up{Fleur::Vec3(0.0f, 1.0f, 0.0f)};

    uint16_t m_FOV{60};

    float m_NearClip{0.1f};
    float m_FarClip{1000.f};

    float m_Yaw{0.f};
    float m_Pitch{0.f};
    // float m_MouseSensitivity{10.f};
    float m_MouseSensitivity{0.2f};
};
}  // namespace Fleur::Graphics
