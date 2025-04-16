#pragma once

namespace Fuego::Renderer
{
class CommandBuffer;
class CommandQueue;

class CommandPool
{
public:
    virtual ~CommandPool() = default;
};
}  // namespace Fuego::Renderer
