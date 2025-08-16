#pragma once

#include <Layer.h>

namespace Fleur
{
class FLEUR_API LayerStack
{
    FLEUR_INTERFACE(LayerStack)

public:
    LayerStack();
    ~LayerStack();

    FLEUR_NON_COPYABLE_NON_MOVABLE(LayerStack)

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);
    void PopLayer(Layer* layer);
    void PopOverlay(Layer* overlay);

    std::vector<Layer*>::iterator begin();
    std::vector<Layer*>::iterator end();
};
}  // namespace Fleur
