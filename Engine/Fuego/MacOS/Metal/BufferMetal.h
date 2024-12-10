#pragma once

#include <Metal/Metal.hpp>

#include "Renderer/Buffer.h"

namespace Fuego::Renderer
{
class BufferMetal : public Buffer
{
public:
    BufferMetal(MTL::Buffer* buffer);

private:
    MTL::Buffer* _buffer;
};
}  // namespace Fuego::Renderer
