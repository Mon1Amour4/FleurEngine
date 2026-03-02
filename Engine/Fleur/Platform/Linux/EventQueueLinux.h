#pragma once

#include <mutex>

#include "EventQueue.h"

namespace Wayland
{
struct Context;
}

namespace Fleur
{
class EventQueueLinux final : public EventQueue
{
    friend class WindowLinux;
    friend struct Wayland::Context;

public:
    virtual void OnUpdate(float dtTime) override;
    virtual void OnPostUpdate(float dtTime) override;

    virtual void OnFixedUpdate() override;

    virtual std::shared_ptr<EventVariant> Front() override;
    virtual void Pop() override;
    virtual bool Empty() override;

    virtual void PushEvent(std::shared_ptr<EventVariant>&& e);

private:
    std::queue<std::shared_ptr<EventVariant>, std::deque<std::shared_ptr<EventVariant>>> m_Queue;
    std::mutex m_Mutex;
};
}  // namespace Fleur
