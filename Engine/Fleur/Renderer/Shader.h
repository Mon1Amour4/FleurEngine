#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Fleur::Graphics
{

class Shader
{
public:
    Shader() = default;
    Shader(const void* byteCode, size_t sizeBytes);

    ~Shader() = default;

    const void* Data() const;
    size_t SizeBytes() const;
    bool Empty() const;

private:
    std::vector<uint8_t> m_ByteCode;
};

}  // namespace Fleur::Graphics
