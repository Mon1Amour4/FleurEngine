#pragma once

#include <span>

namespace Fleur::Graphics
{
class Buffer
{
public:
    virtual ~Buffer() = default;

    enum EBufferType
    {
        Vertex,
        Index
    };

    size_t UpdateSubData(const void* data, size_t sizeBytes)
    {
        return UpdateSubDataImpl(data, sizeBytes);
    }
    virtual uint32_t NativeType() const = 0;

protected:
    virtual size_t UpdateSubDataImpl(const void* data, size_t sizeBytes) = 0;

    Buffer(EBufferType type, size_t sizeBytes)
        : m_Type(type)
        , m_EndIdx(sizeBytes)
        , m_UsedBytesIdx(0) {};
    size_t m_EndIdx;
    size_t m_UsedBytesIdx;
    EBufferType m_Type;
};
}  // namespace Fleur::Graphics
