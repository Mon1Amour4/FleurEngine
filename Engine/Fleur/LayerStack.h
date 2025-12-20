#pragma once

#include "FleurAllocator.hpp"
#include "Layer.h"

namespace Fleur
{
class FLEUR_API LayerStack
{
    FLEUR_INTERFACE(LayerStack)
    using vector = std::vector<Layer*, Fleur::Memory::FleurAllocator<Layer*>>;

public:
    LayerStack();
    ~LayerStack();

    FLEUR_NON_COPYABLE_NON_MOVABLE(LayerStack)

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);
    void PopLayer(Layer* layer);
    void PopOverlay(Layer* overlay);

    [[nodiscard]] vector::iterator begin();
    [[nodiscard]] vector::iterator end();
};
}  // namespace Fleur
