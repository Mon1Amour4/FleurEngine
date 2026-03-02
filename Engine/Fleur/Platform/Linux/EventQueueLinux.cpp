#include "EventQueueLinux.h"

void Fleur::EventQueueLinux::OnUpdate(float dtTime)
{
    UNUSED(dtTime);
    // TODO
}

void Fleur::EventQueueLinux::OnPostUpdate(float dtTime)
{
    UNUSED(dtTime);
    // TODO
}

void Fleur::EventQueueLinux::OnFixedUpdate()
{
    // TODO
}

std::shared_ptr<Fleur::EventVariant> Fleur::EventQueueLinux::Front()
{
    std::lock_guard lock(m_Mutex);
    return m_Queue.front();
}

void Fleur::EventQueueLinux::Pop()
{
    std::lock_guard lock(m_Mutex);
    m_Queue.pop();
}

bool Fleur::EventQueueLinux::Empty()
{
    return m_Queue.empty();
}

void Fleur::EventQueueLinux::PushEvent(std::shared_ptr<EventVariant>&& e)
{
    std::lock_guard lock(m_Mutex);
    m_Queue.push(std::move(e));
}

std::unique_ptr<Fleur::EventQueue> Fleur::EventQueue::CreateEventQueue()
{
    return std::make_unique<EventQueueLinux>();
}
