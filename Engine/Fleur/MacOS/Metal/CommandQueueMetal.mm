
#include "CommandQueueMetal.h"

namespace Fleur::Renderer
{
CommandQueueMetal::CommandQueueMetal(MTL::CommandQueue* commandQueue)
    : _commandQueue(commandQueue)
{
}

}  // namespace Fleur::Renderer
