#pragma once

#include <chrono>
#include <cstdint>

namespace Fleur
{
// Frame clock. Portable: std::chrono::steady_clock is monotonic and already maps
// to the platform's best high-resolution source (QueryPerformanceCounter on
// Windows, clock_gettime(CLOCK_MONOTONIC) on Linux/macOS), so no per-platform
// implementation is needed.
class Time
{
public:
    explicit Time(float fixed_time)
        : m_FixedTime(fixed_time)
        , m_Timer(std::chrono::steady_clock::now())
    {
    }

    // Advances the clock by one frame. The caller passes whether the window is
    // active; when inactive, delta time is forced to 0 (sim pauses). Passing it in
    // keeps Time decoupled from Application/Window.
    void Tick(bool windowActive);

    inline uint64_t Frame() const
    {
        return m_Frame;
    }
    inline float DeltaTime() const
    {
        return m_DeltaTime;
    }
    inline float FixedTime() const
    {
        return m_FixedTime;
    }
    inline float FPS() const
    {
        return m_FPS;
    }
    inline float AverageFrameTime() const
    {
        return m_AverageFrametime;
    }

private:
    void CalcDeltaTime(bool windowActive);

    std::chrono::steady_clock::time_point m_Timer;

    uint64_t m_Frame{0};
    float m_FixedTime;
    float m_DeltaTime{0};
    float m_FPS{0};
    float m_FPSTimer{0};
    uint16_t m_FPSFrames{0};

    float m_AverageFrametime{0};

    uint32_t m_ApplicationEpoch{0};
};
}  // namespace Fleur
