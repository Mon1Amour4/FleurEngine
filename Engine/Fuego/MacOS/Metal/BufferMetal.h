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
    virtual void BindDataImpl(const void* data, size_t size, size_t offset) override;

    MTL::Buffer* _buffer;
};
}  // namespace Fuego::Renderer
