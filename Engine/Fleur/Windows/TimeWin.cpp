#include "TimeWin.h"

#include "Application.h"

std::unique_ptr<Fleur::Time> Fleur::Time::CreateTimeManager(float fixed_time)
{
    return std::make_unique<Fleur::TimeWin>(fixed_time);
}

Fleur::TimeWin::TimeWin(float fixed_time)
    : Time(fixed_time)
    , timer(std::chrono::steady_clock::now())
{
}

void Fleur::TimeWin::Tick()
{
    calc_delta_time();
    ++m_Frame;
}

void Fleur::TimeWin::calc_delta_time()
{
    auto now = std::chrono::steady_clock::now();
    m_DeltaTime = std::chrono::duration<float>(now - timer).count();
    m_FPSTime += m_DeltaTime;
    ++m_FPSFrames;
    if (m_FPSTime >= 1.f)
    {
        m_FPS = m_FPSFrames / m_FPSTime;
        m_AverageFrametime = m_FPSTime / m_FPSFrames;
        m_ApplicationEpoch++;
        m_FPSTime = 0;
        m_FPSFrames = 0;
        FL_CORE_TRACE("second: {0}, average_frametime: {1}", m_ApplicationEpoch, m_AverageFrametime);
    }
    timer = now;

    if (!Application::instance().GetWindow().IsActive())
        m_DeltaTime = 0.f;
}
