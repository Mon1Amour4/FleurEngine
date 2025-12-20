#include "LayerStack.h"

class Fleur::LayerStack::LayerStackImpl
{
    friend class LayerStack;

    vector m_Layers;
    vector::iterator m_LayerInsert;
};

Fleur::LayerStack::LayerStack()
    : d(new LayerStackImpl)
{
    d->m_LayerInsert = d->m_Layers.begin();
}

Fleur::LayerStack::~LayerStack()
{
    for (auto layer : d->m_Layers)
    {
        delete layer;
    }

    delete d;
}

void Fleur::LayerStack::PushLayer(Layer* layer)
{
    d->m_LayerInsert = d->m_Layers.emplace(d->m_LayerInsert, layer);
}

void Fleur::LayerStack::PushOverlay(Layer* overlay)
{
    d->m_Layers.emplace_back(overlay);
}

void Fleur::LayerStack::PopLayer(Layer* layer)
{
    auto it = std::ranges::find(d->m_Layers.begin(), d->m_Layers.end(), layer);
    if (it != d->m_Layers.end())
    {
        d->m_Layers.erase(it);
        --d->m_LayerInsert;
    }
}

void Fleur::LayerStack::PopOverlay(Layer* overlay)
{
    auto it = std::ranges::find(d->m_Layers.begin(), d->m_Layers.end(), overlay);
    if (it != d->m_Layers.end())
    {
        d->m_Layers.erase(it);
    }
}

Fleur::LayerStack::vector::iterator Fleur::LayerStack::begin()
{
    return d->m_Layers.begin();
}

Fleur::LayerStack::vector::iterator Fleur::LayerStack::end()
{
    return d->m_Layers.end();
}
