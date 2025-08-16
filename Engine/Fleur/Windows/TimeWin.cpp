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
    ++frame;
}

void Fleur::TimeWin::calc_delta_time()
{
    auto now = std::chrono::steady_clock::now();
    delta_time = std::chrono::duration<float>(now - timer).count();
    fps_time += delta_time;
    ++fps_frames;
    if (fps_time >= 1.f)
    {
        fps = fps_frames / fps_time;
        average_frametime = fps_time / fps_frames;
        application_epoch++;
        fps_time = 0;
        fps_frames = 0;
        FL_CORE_TRACE("second: {0}, average_frametime: {1}", application_epoch, average_frametime);
    }
    timer = now;

    if (!Application::instance().GetWindow().IsActive())
        delta_time = 0.f;
}
