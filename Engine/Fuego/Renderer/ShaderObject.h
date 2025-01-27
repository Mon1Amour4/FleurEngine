#pragma once

namespace Fuego::Renderer
{
class Shader;

class ShaderObject
{
public:
    virtual ~ShaderObject() = default;

    static ShaderObject* CreateShaderObject(Shader& vs, Shader& px);
    virtual Shader* GetPixelShader() = 0;
    virtual Shader* GetVertexShader() = 0;
};
}  // namespace Fuego::Renderer