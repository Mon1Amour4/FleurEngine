#pragma once

namespace Fuego::Renderer
{
class Shader;

class ShaderObject
{
public:
    virtual ~ShaderObject();

    static void CreateShaderObject(Shader& vertex_shader, Shader& pixel_shader);
    virtual void LinkShaders() = 0;
};
}  // namespace Fuego::Renderer