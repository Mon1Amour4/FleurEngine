#include "CommandPoolMetal.h"

#include "CommandBufferMetal.h"
#include "CommandQueueMetal.h"

namespace Fuego::Renderer
{

CommandPoolMetal::CommandPoolMetal(const CommandQueue& queue)
{
    _commandQueue = dynamic_cast<const CommandQueueMetal*>(&queue)->_commandQueue;
}

std::unique_ptr<CommandBuffer> CommandPoolMetal::CreateCommandBuffer()
{
    return std::make_unique<CommandBufferMetal>(_commandQueue->commandBuffer());
}

}  // namespace Fuego::Renderer
