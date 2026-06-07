#pragma once

namespace Fleur::Graphics
{
class CommandBuffer;
class CommandQueue;

class CommandPool
{
public:
    virtual ~CommandPool() = default;
};
}  // namespace Fleur::Graphics
