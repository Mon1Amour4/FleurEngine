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
    ~Camera();
    FLEUR_NON_COPYABLE_NON_MOVABLE(Camera);

    inline mat4 GetView() const
    {
        return view;
    }
    float Yaw() const
    {
        return yaw;
    }
    float Pitch() const
    {
        return pitch;
    }
    inline mat4 GetProjection() const
    {
        return projection;
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
    float speed;
    void RotateCamera(float dtTime);
    static Camera* active_camera;
    vec3 position;
    mat4 view;
    mat4 projection;
    vec3 dir;
    vec3 camera_forward;
    vec3 up;
    uint16_t FOV;
    float near_clip;
    float far_clip;

    float yaw;
    float pitch;
    float mouse_sensitivity;

    friend class Renderer;
    Camera();
};
}  // namespace Fleur::Graphics
