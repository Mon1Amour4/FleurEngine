#pragma once

namespace Fleur::Graphics
{
class CommandBuffer;

class CommandQueue
{
public:
    virtual ~CommandQueue() = default;

    virtual void Submit(const CommandBuffer& commandBuffer) = 0;
    virtual void Wait() = 0;
};
}  // namespace Fleur::Graphics
