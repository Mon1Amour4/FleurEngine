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

CommandPoolMetal::~CommandPoolMetal()
{
}

CommandBuffer& CommandPoolMetal::GetCommandBuffer()
{
}

}  // namespace Fuego::Renderer
