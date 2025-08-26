#include "EventQueueWin.h"

namespace Fleur
{
void EventQueueWin::OnUpdate(float dtTime)
{
    // TODO
}

void EventQueueWin::OnPostUpdate(float dlTime)
{
    // TODO
}

void EventQueueWin::OnFixedUpdate()
{
    // TODO
}

std::shared_ptr<EventVariant> EventQueueWin::Front()
{
    std::lock_guard lock(m_Mutex);
    return m_Queue.front();
}

void EventQueueWin::Pop()
{
    std::lock_guard lock(m_Mutex);
    m_Queue.pop();
}

bool EventQueueWin::Empty()
{
    return m_Queue.empty();
}

void EventQueueWin::PushEvent(std::shared_ptr<EventVariant>&& e)
{
    std::lock_guard lock(m_Mutex);
    m_Queue.push(std::move(e));
}

std::unique_ptr<EventQueue> EventQueue::CreateEventQueue()
{
    return std::make_unique<EventQueueWin>();
}
}  // namespace Fleur
