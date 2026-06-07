#pragma once

#include <Metal/Metal.hpp>

#include "Lux/Buffer.h"

namespace Fleur::Renderer
{
class BufferMetal : public Buffer
{
public:
    friend class CommandBufferMetal;

    BufferMetal(MTL::Buffer* buffer);

private:
    virtual void BindDataImpl(const void* data, size_t size, size_t offset) override;

    MTL::Buffer* _buffer;
};
}  // namespace Fleur::Renderer
