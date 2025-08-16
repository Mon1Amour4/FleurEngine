#pragma once

#include "Core.h"
#include "Events/EventVisitor.h"

namespace Fleur
{
class FLEUR_API EventQueue : public IUpdatable
{
public:
    EventQueue() = default;
    FLEUR_NON_COPYABLE_NON_MOVABLE(EventQueue)

    virtual void OnUpdate(float dlTime) = 0;
    virtual void OnPostUpdate(float dlTime) = 0;
    virtual void OnFixedUpdate() = 0;

    virtual std::shared_ptr<EventVariant> Front() = 0;
    virtual void Pop() = 0;
    virtual bool Empty() = 0;

    virtual ~EventQueue() = default;

    static std::unique_ptr<EventQueue> CreateEventQueue();
};
}  // namespace Fleur
