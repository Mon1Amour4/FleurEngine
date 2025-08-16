#pragma once

#include "Renderer/CommandPool.h"

namespace Fleur::Graphics
{
class CommandQueue;
class CommandBufferOpenGL;

class CommandPoolOpenGL final : public CommandPool
{
public:
    FLEUR_NON_COPYABLE_NON_MOVABLE(CommandPoolOpenGL)

    virtual ~CommandPoolOpenGL() override = default;

protected:
    friend class DeviceOpenGL;
    explicit CommandPoolOpenGL(const CommandQueue& queue);
};
}  // namespace Fleur::Graphics
