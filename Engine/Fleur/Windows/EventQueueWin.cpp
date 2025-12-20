#include "EventQueueWin.h"

void Fleur::EventQueueWin::OnUpdate(float dtTime)
{
    UNUSED(dtTime);
    // TODO
}

void Fleur::EventQueueWin::OnPostUpdate(float dtTime)
{
    UNUSED(dtTime);
    // TODO
}

void Fleur::EventQueueWin::OnFixedUpdate()
{
    // TODO
}

std::shared_ptr<Fleur::EventVariant> Fleur::EventQueueWin::Front()
{
    std::lock_guard lock(m_Mutex);
    return m_Queue.front();
}

void Fleur::EventQueueWin::Pop()
{
    std::lock_guard lock(m_Mutex);
    m_Queue.pop();
}

bool Fleur::EventQueueWin::Empty()
{
    return m_Queue.empty();
}

void Fleur::EventQueueWin::PushEvent(std::shared_ptr<EventVariant>&& e)
{
    std::lock_guard lock(m_Mutex);
    m_Queue.push(std::move(e));
}

std::unique_ptr<Fleur::EventQueue> Fleur::EventQueue::CreateEventQueue()
{
    return std::make_unique<EventQueueWin>();
}
