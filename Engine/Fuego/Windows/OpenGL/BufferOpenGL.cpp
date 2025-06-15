#include "BufferOpenGL.h"

#include "../../Renderer/Graphics.hpp"
#include "glad/gl.h"

namespace Fuego::Graphics
{

BufferOpenGL::BufferOpenGL(BufferType type, RenderStage stage, size_t size_bytes)
    : Buffer(type, size_bytes), buffer_object_id(UINT32_MAX)
{
    FU_CORE_ASSERT(size_bytes > 0, "Buffer can't be 0 sized");

    glGenBuffers(1, &buffer_object_id);

    buffer_native_type = native_buffer_type(type);

    glBindBuffer(buffer_native_type, buffer_object_id);
    glBufferData(buffer_native_type, size_bytes, nullptr, native_usage(stage));
    glBindBuffer(buffer_native_type, 0);
}

BufferOpenGL::~BufferOpenGL()
{
    glBindBuffer(buffer_native_type, 0);
    glDeleteBuffers(1, &buffer_object_id);
}

uint32_t BufferOpenGL::UpdateSubDataImpl(const void* data, size_t size_bytes)
{
    FU_CORE_ASSERT(last_buffered_idx_to_byte + size_bytes < end_idx, "Buffer overflow");

    uint32_t offset_before_write = last_buffered_idx_to_byte;

    glBindBuffer(buffer_native_type, buffer_object_id);
    glBufferSubData(buffer_native_type, last_buffered_idx_to_byte, size_bytes, data);
    glBindBuffer(buffer_native_type, 0);
    last_buffered_idx_to_byte += size_bytes;
    return offset_before_write;
}

uint32_t BufferOpenGL::NativeType() const
{
    return buffer_native_type;
}

uint32_t BufferOpenGL::GetBufferID() const
{
    return buffer_object_id;
}

int BufferOpenGL::native_usage(RenderStage& stage) const
{
    switch (stage)
    {
        case STATIC_GEOMETRY:
            return GL_STATIC_DRAW;
        case DYNAMIC_DRAW:
            return GL_DYNAMIC_DRAW;
    }
}
int BufferOpenGL::native_buffer_type(const BufferType& type) const
{
    if (type == BufferType::Vertex)
        return GL_ARRAY_BUFFER;
    else
        return GL_ELEMENT_ARRAY_BUFFER;
}
}  // namespace Fuego::Graphics
