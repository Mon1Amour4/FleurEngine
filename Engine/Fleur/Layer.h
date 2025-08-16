#pragma once

#include <Events/Event.h>

#include "Events/EventVisitor.h"

namespace Fleur
{
class FLEUR_API Layer : IUpdatable
{
    FLEUR_INTERFACE(Layer)

public:
    Layer(const std::string& name = "Layer");
    virtual ~Layer();

    virtual void OnAttach();
    virtual void OnDetach();

    virtual void OnUpdate(float dlTime);
    virtual void OnPostUpdate(float dlTime);
    virtual void OnFixedUpdate();

    virtual void OnEvent(EventVariant& event);

    const std::string& GetName() const;
};
}  // namespace Fleur
