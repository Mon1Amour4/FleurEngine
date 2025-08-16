
#pragma once

#include <Metal/Metal.hpp>

#include "Renderer/CommandQueue.h"

namespace Fleur::Renderer
{
class CommandQueueMetal : public CommandQueue
{
public:
    friend class DeviceMetal;

    CommandQueueMetal(MTL::CommandQueue* commandQueue);

    virtual void Submit(const CommandBuffer& commandBuffer)
    {
    }
    virtual void Wait()
    {
    }

private:
    MTL::CommandQueue* _commandQueue;
};
}  // namespace Fleur::Renderer
