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

class CommandPoolMetal : public CommandPool
{
public:
    CommandPoolMetal(MTL::CommandQueue* commandQueue, MTL::Device* device);
    virtual ~CommandPoolMetal() = default;

    virtual std::unique_ptr<CommandBuffer> CreateCommandBuffer() override;

private:
    MTL::CommandQueue* _commandQueue;
    MTL::Device* _device;
};
}  // namespace Fuego::Renderer
