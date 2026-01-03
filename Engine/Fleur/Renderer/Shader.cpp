#include "Shader.h"

Fleur::Graphics::Shader::Shader(const char* shaderCode, size_t size)
    : byteCode(size)
{
    memmove(byteCode.data(), shaderCode, size);
}

const char* Fleur::Graphics::Shader::GetShaderCode() const
{
    return byteCode.data();
}

uint32_t Fleur::Graphics::Shader::GetShaderCodeSizeB() const
{
    return byteCode.size();
}
