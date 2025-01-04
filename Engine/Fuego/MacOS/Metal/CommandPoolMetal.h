#pragma once

#include "Renderer/CommandPool.h"

namespace MTL
{
class CommandQueue;
class Device;
}  // namespace MTL

namespace Fuego::Renderer
{
class CommandQueue;
class CommandBufferMetal;

class CommandPoolMetal : public CommandPool
{
public:
    CommandPoolMetal(MTL::CommandQueue* commandQueue, MTL::Device* device);
    virtual ~CommandPoolMetal();

    virtual CommandBuffer& GetCommandBuffer() override;

private:
    MTL::CommandQueue* _commandQueue;
    MTL::Device* _device;
    std::vector<CommandBufferMetal*> _cmdBuffers;
};
}  // namespace Fuego::Renderer
