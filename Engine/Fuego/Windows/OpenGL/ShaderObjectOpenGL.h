#pragma once

#include "ShaderObject.h"
#include "ShaderOpenGL.h"

namespace Fuego::Renderer
{
class ShaderObjectOpenGL : public ShaderObject
{
public:
    virtual ~ShaderObjectOpenGL() override;

    inline virtual Shader* GetPixelShader() override
    {
        return pixel_shader;
    }
    inline virtual Shader* GetVertexShader() override
    {
        return vertex_shader;
    }
    inline virtual uint16_t GetObjectID()
    {
        return program;
    }

    void Use() const;

private:
    uint16_t program;
    ShaderOpenGL* vertex_shader;
    ShaderOpenGL* pixel_shader;

    friend class ShaderObject;
    ShaderObjectOpenGL(Shader& vs, Shader& px);
};
}  // namespace Fuego::Renderer