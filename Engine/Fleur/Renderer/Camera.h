#pragma once

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include "Services/ServiceInterfaces.hpp"

using vec3 = glm::vec3;
using mat4 = glm::mat4;

namespace Fleur::Graphics
{
class Renderer;

class FLEUR_API Camera : IUpdatable
{
public:
    Camera();
    ~Camera();

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

    const mat4* GetViewPtr() const;
    vec3 GetDir() const;
    static Camera* GetActiveCamera();

    void Activate();

    void OnUpdate(float dlTime);
    void OnPostUpdate(float dlTime);
    void OnFixedUpdate();

    float FarClip() const;
    float NearClip() const;

private:
    float m_Speed;
    void UpdateForward();
    void RotateCamera(float dtTime);
    static Camera* s_ActiveCamera;
    vec3 m_Position;
    mat4 m_View;
    mat4 m_Projection;
    vec3 m_Dir;
    vec3 m_CameraForward;
    vec3 m_Up;
    uint16_t m_FOV;
    float m_NearClip;
    float m_FarClip;

    float m_Yaw;
    float m_Pitch;
    float m_MouseSensitivity;
};
}  // namespace Fleur::Graphics
