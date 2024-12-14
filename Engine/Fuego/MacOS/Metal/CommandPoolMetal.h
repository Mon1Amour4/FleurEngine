#pragma once

#include "Renderer/CommandPool.h"

namespace MTL
{
class CommandQueue;
}

namespace Fuego::Renderer
{
class CommandQueue;

class CommandPoolMetal : public CommandPool
{
public:
    CommandPoolMetal(const CommandQueue& queue);
    virtual ~CommandPoolMetal() = default;

    virtual std::unique_ptr<CommandBuffer> CreateCommandBuffer() override;

private:
    MTL::CommandQueue* _commandQueue;
};
}  // namespace Fuego::Renderer
