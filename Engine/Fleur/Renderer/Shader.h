#pragma once

namespace Fleur::Graphics
{

class Shader
{
public:
    Shader(std::string shaderCode);

    ~Shader() = default;

    const char* GetShaderCode() const;

private:
    std::string byteCode;
};

}  // namespace Fleur::Graphics
