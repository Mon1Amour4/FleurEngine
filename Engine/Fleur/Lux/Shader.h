#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Fleur::Graphics
{

class Shader
{
public:
    Shader() = default;
    Shader(const char* shaderCode, size_t size);

    ~Shader() = default;

    const char* GetShaderCode() const;
    uint32_t GetShaderCodeSizeB() const;

private:
    std::vector<char> byteCode;
};

using ShaderRegistry = std::unordered_map<std::string, Shader>;

}  // namespace Fleur::Graphics
