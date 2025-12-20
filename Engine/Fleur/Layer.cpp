#include "Layer.h"

class Fleur::Layer::LayerImpl
{
    friend class Layer;

    std::string m_Name;
};

Fleur::Layer::Layer(const std::string& name)
    : d(new LayerImpl)
{
    d->m_Name = name;
}

Fleur::Layer::~Layer()
{
    delete d;
}

void Fleur::Layer::OnAttach()
{
}

void Fleur::Layer::OnDetach()
{
}

void Fleur::Layer::OnUpdate(float dtTime)
{
    UNUSED(dtTime);
    // TODO
}

void Fleur::Layer::OnPostUpdate(float dtTime)
{
    UNUSED(dtTime);
    // TODO
}

void Fleur::Layer::OnFixedUpdate()
{
    // TODO
}

void Fleur::Layer::OnEvent(EventVariant& event)
{
    UNUSED(event);
}

const std::string& Fleur::Layer::GetName() const
{
    return d->m_Name;
}
