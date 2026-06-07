#pragma once

#pragma region Concepts

template <typename Derived>
concept is_service_interface = requires(Derived t) {
    { t.Init() } -> std::same_as<void>;
    { t.Shutdown() } -> std::same_as<void>;
};

#pragma endregion

namespace Fleur::Graphics
{
class Model;
class Texture;
class Renderer;
}  // namespace Fleur::Graphics

namespace Fleur
{

struct IUpdatable
{
    virtual ~IUpdatable() = default;
    virtual void OnUpdate(float dtTime) {}
    virtual void OnPostUpdate(float dtTime) {}
    virtual void OnFixedUpdate() {}
};

struct IInitializable
{
    virtual void Init() = 0;
    virtual void Shutdown() = 0;
};

template <class Derived>
struct Service : public IInitializable
{
    void Init()
    {
        if (m_IsInitialized)
            return;
        Derived* derived = static_cast<Derived*>(this);
        derived->OnInit();
        m_IsInitialized = true;
    };

    void Shutdown()
    {
        if (m_IsShutdown)
            return;
        Derived* derived = static_cast<Derived*>(this);
        derived->OnShutdown();
        m_IsShutdown = true;
    }

protected:
    inline static bool m_IsInitialized = false;
    inline static bool m_IsShutdown = false;
};

}  // namespace Fleur
