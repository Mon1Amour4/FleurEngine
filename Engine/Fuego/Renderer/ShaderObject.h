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
    virtual void Use() const = 0;
    virtual void BindMaterial(Material* material) = 0;
    virtual void UseMaterial() const = 0;
};
}  // namespace Fuego::Renderer