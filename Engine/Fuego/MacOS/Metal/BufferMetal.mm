#include "BufferMetal.h"

namespace Fuego::Renderer
{
BufferMetal::BufferMetal(MTL::Buffer* buffer)
    : _buffer(buffer)
{
}

void BufferMetal::BindDataImpl(const void* data, size_t size, size_t offset)
{
    FU_CORE_ASSERT(!_buffer || !data || offset + size > _buffer->length(), "Metal Buffer is invalid")

    std::memcpy(static_cast<uint8_t*>(_buffer->contents()) + offset, data, size);
}

}  // namespace Fuego::Renderer
