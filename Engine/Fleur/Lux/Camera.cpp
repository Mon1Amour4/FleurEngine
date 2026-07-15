#include "Camera.h"

#include "Input.h"

#define MOUSE_WHEEL_SCROLL_SPEED 5.f
static constexpr float MOUSE_EPSILON = 1e-3f;

Fleur::Graphics::Camera* Fleur::Graphics::Camera::s_ActiveCamera = nullptr;

Fleur::Graphics::Camera::Camera()
    : m_Up(Fleur::Math::vec3(0.0f, 1.0f, 0.0f))
    , m_CameraForward(0.0f, 0.0f, -1.0f)
    , m_View(Fleur::Math::mat4(1.0f))
    , m_Projection(Fleur::Math::mat4(1.0f))
{
    m_Projection = Fleur::Math::perspective(Fleur::Math::radians((float)m_FOV), 1280.0F / 720.0F, m_NearClip, m_FarClip);
    UpdateForward();
}

void Fleur::Graphics::Camera::Activate()
{
    s_ActiveCamera = this;
}

const mat4* Fleur::Graphics::Camera::GetViewPtr() const
{
    return &m_View;
}

Fleur::Graphics::Camera* Fleur::Graphics::Camera::GetActiveCamera()
{
    return s_ActiveCamera;
}

void Fleur::Graphics::Camera::OnUpdate(float dtTime)
{
    if (Input::IsKeyPressed(Key::W))
    {
        m_Position += m_Speed * m_CameraForward * dtTime;
    }
    if (Input::IsKeyPressed(Key::S))
    {
        m_Position -= m_Speed * m_CameraForward * dtTime;
    }
    if (Input::IsKeyPressed(Key::A))
    {
        m_Position -= Fleur::Math::normalize(Fleur::Math::cross(m_CameraForward, m_Up)) * m_Speed * dtTime;
    }
    if (Input::IsKeyPressed(Key::D))
    {
        m_Position += Fleur::Math::normalize(Fleur::Math::cross(m_CameraForward, m_Up)) * m_Speed * dtTime;
    }

    std::pair<int, int> mouseWheelScroll;
    if (Input::IsMouseWheelScrolled(mouseWheelScroll))
    {
        if (mouseWheelScroll.first != 0.f)
        {
            if (mouseWheelScroll.first > 0.f)
                m_Speed += MOUSE_WHEEL_SCROLL_SPEED;
            else
                m_Speed -= MOUSE_WHEEL_SCROLL_SPEED;
            m_Speed = std::clamp(m_Speed, 1.f, 300.f);
        }
    }

    RotateCamera(dtTime);

    m_View = Fleur::Math::lookAt(m_Position, m_Position + m_CameraForward, m_Up);
}

void Fleur::Graphics::Camera::OnPostUpdate(float dtTime)
{
    UNUSED(dtTime);
    // TODO
}

void Fleur::Graphics::Camera::OnFixedUpdate()
{
    // TODO
}

float Fleur::Graphics::Camera::FarClip() const
{
    return m_FarClip;
}

float Fleur::Graphics::Camera::NearClip() const
{
    return m_NearClip;
}

void Fleur::Graphics::Camera::UpdateForward()
{
    Fleur::Math::vec3 direction;
    direction.x = cos(Fleur::Math::radians(m_Yaw)) * cos(Fleur::Math::radians(m_Pitch));
    direction.y = sin(Fleur::Math::radians(m_Pitch));
    direction.z = sin(Fleur::Math::radians(m_Yaw)) * cos(Fleur::Math::radians(m_Pitch));
    m_CameraForward = Fleur::Math::normalize(direction);
}

void Fleur::Graphics::Camera::RotateCamera(float dtTime)
{
    Fleur::Math::vec2 mouseDir = Input::GetMouseDir();
    if (fabs(mouseDir.x) > MOUSE_EPSILON || fabs(mouseDir.y) > MOUSE_EPSILON)
    {
        m_Yaw += mouseDir.x * m_MouseSensitivity;            //* dtTime;
        m_Pitch += mouseDir.y * -1.0f * m_MouseSensitivity;  //* dtTime;
        m_Pitch = Fleur::Math::fclamp(m_Pitch, -89.0f, 89.0f);

        UpdateForward();
    }
}
