#pragma once

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

}  // namespace Fleur::Graphics
