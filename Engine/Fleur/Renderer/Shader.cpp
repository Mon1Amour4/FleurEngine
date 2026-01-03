#include "Shader.h"

Fleur::Graphics::Shader::Shader(std::string shaderCode)
    : byteCode(shaderCode)
{
}

const char* Fleur::Graphics::Shader::GetShaderCode() const
{
    return byteCode.c_str();
}

uint32_t Fleur::Graphics::Shader::GetShaderCodeSizeB() const
{
    return byteCode.length();
}
