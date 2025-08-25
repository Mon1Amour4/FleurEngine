#pragma once

namespace Fleur::Graphics
{
class ShaderObject;
class Texture;

class Shader
{
public:
    enum ShaderType
    {
        None = 0,
        Vertex = 1,
        Pixel = 2
    };

    friend class ShaderObject;
    virtual void BindToShaderObject(ShaderObject& obj) = 0;

    virtual ~Shader() = default;

    virtual void Release() = 0;

    protected:
    Shader(std::string_view name)
        : m_Name(name)
    {
    }
        std::string m_Name;
};
}  // namespace Fleur::Graphics
