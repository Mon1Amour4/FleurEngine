#include "Shader.h"

#include <cstring>

Fleur::Graphics::Shader::Shader(const void* byteCode, size_t sizeBytes)
    : m_ByteCode(sizeBytes)
{
    if (byteCode && sizeBytes > 0)
        std::memcpy(m_ByteCode.data(), byteCode, sizeBytes);
}

const void* Fleur::Graphics::Shader::Data() const
{
    return m_ByteCode.data();
}

size_t Fleur::Graphics::Shader::SizeBytes() const
{
    return m_ByteCode.size();
}

bool Fleur::Graphics::Shader::Empty() const
{
    return m_ByteCode.empty();
}
