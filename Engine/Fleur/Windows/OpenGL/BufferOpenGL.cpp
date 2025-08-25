#include "BufferOpenGL.h"

#include "../../Renderer/Graphics.hpp"
#include "glad/gl.h"

namespace Fleur::Graphics
{

BufferOpenGL::BufferOpenGL(EBufferType type, RenderStage stage, size_t sizeBytes)
    : Buffer(m_Type, sizeBytes)
    , m_Id(UINT32_MAX)
{
    FL_CORE_ASSERT(sizeBytes > 0, "Buffer can't be 0 sized");

    glCreateBuffers(1, &m_Id);
    glNamedBufferData(m_Id, sizeBytes, nullptr, native_usage(stage));
}

BufferOpenGL::~BufferOpenGL()
{
    glDeleteBuffers(1, &m_Id);
}

uint32_t BufferOpenGL::UpdateSubDataImpl(const void* data, size_t sizeBytes)
{
    FL_CORE_ASSERT(m_UsedBytesIdx + sizeBytes <= m_EndIdx, "Buffer overflow");

    uint32_t offset_before_write = m_UsedBytesIdx;

    glNamedBufferSubData(m_Id, m_UsedBytesIdx, sizeBytes, data);
    m_UsedBytesIdx += sizeBytes;
    return offset_before_write;
}

uint32_t BufferOpenGL::NativeType() const
{
    return m_BufferNativeType;
}

uint32_t BufferOpenGL::GetBufferID() const
{
    return m_Id;
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
int BufferOpenGL::native_buffer_type(const EBufferType& type) const
{
    if (type == EBufferType::Vertex)
        return GL_ARRAY_BUFFER;
    else
        return GL_ELEMENT_ARRAY_BUFFER;
}
}  // namespace Fleur::Graphics
