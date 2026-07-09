#include "FlTime.h"

#include "Log.h"

namespace Fleur
{
void Time::Tick(bool windowActive)
{
    CalcDeltaTime(windowActive);
    ++m_Frame;
}

void Time::CalcDeltaTime(bool windowActive)
{
    auto now = std::chrono::steady_clock::now();
    m_DeltaTime = std::chrono::duration<float>(now - m_Timer).count();
    m_Timer = now;

    m_FPSTimer += m_DeltaTime;
    ++m_FPSFrames;
    if (m_FPSTimer >= 1.f)
    {
        m_FPS = m_FPSFrames / m_FPSTimer;
        m_AverageFrametime = m_FPSTimer / m_FPSFrames;
        ++m_ApplicationEpoch;
        m_FPSTimer = 0;
        m_FPSFrames = 0;
        FL_CORE_TRACE("second: {0}, average_frametime: {1}", m_ApplicationEpoch, m_AverageFrametime);
    }

    if (!windowActive)
        m_DeltaTime = 0.f;
}
}  // namespace Fleur
