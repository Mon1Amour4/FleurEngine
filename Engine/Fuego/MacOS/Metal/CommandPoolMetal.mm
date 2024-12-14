#include "CommandPoolMetal.h"

#include "CommandBufferMetal.h"
#include "CommandQueueMetal.h"

namespace Fuego::Renderer
{

CommandPoolMetal::CommandPoolMetal(MTL::CommandQueue* commandQueue, MTL::Device* device)
    : _commandQueue(commandQueue)
    , _device(device)
{
}

std::unique_ptr<CommandBuffer> CommandPoolMetal::CreateCommandBuffer()
{
    return std::make_unique<CommandBufferMetal>(_commandQueue->commandBuffer(), _device);
}

}  // namespace Fuego::Renderer
