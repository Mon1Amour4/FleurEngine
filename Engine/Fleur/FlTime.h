#pragma once

namespace Fleur
{
class Time
{
public:
    virtual ~Time() = default;
    static std::unique_ptr<Time> CreateTimeManager(float fixed_time);

    virtual void Tick() = 0;

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
    inline uint16_t FPS() const
    {
        return m_FPS;
    }
    inline float AverageFrameTime() const
    {
        return m_AverageFrametime;
    }

protected:
    Time(float fixed_time)
        : m_Frame(0)
        , m_FixedTime(fixed_time)
        , m_DeltaTime(0.f)
        , m_FPS(0)
        , m_FPSTime(0.f)
        , m_FPSFrames(0)
        , m_AverageFrametime(0.f)
        , m_ApplicationEpoch(0) {};

    uint64_t m_Frame;
    float m_FixedTime;
    float m_DeltaTime;
    uint16_t m_FPS;
    float m_FPSTime;
    uint16_t m_FPSFrames;

    float m_AverageFrametime;

    uint32_t m_ApplicationEpoch;
};
}  // namespace Fleur
