#pragma once

#include <chrono>

#include "FlTime.h"

namespace Fleur
{
class TimeWin : public Time
{
public:
    TimeWin(float fixedTime);
    virtual ~TimeWin() override = default;

    virtual void Tick() override;

private:
    std::chrono::time_point<std::chrono::steady_clock> timer;
    void CalcDeltaTime();
};
}  // namespace Fleur
