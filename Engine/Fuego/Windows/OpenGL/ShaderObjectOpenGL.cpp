#include "ShaderObjectOpenGL.h"

namespace Fuego::Renderer
{

ShaderObject* ShaderObject::CreateShaderObject(Shader& vs, Shader& px)
{
    return new ShaderObjectOpenGL(vs, px);
}

ShaderObjectOpenGL::ShaderObjectOpenGL(Shader& vs, Shader& px)
    : vertex_shader(nullptr)
    , pixel_shader(nullptr)
    , program(glCreateProgram())
{
    auto vs_gl = static_cast<ShaderOpenGL*>(&vs);
    auto px_gl = static_cast<ShaderOpenGL*>(&px);
    vertex_shader = vs_gl;
    pixel_shader = px_gl;

    glAttachShader(program, vs_gl->GetID());
    glAttachShader(program, px_gl->GetID());

    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        FU_CORE_ERROR("[ShaderObject] program linking error: ", infoLog);
    }
    vs_gl->BindToShaderObject(*this);
    px_gl->BindToShaderObject(*this);

    glUseProgram(0);
}

void ShaderObjectOpenGL::Use() const
{
    glUseProgram(program);
}

ShaderObjectOpenGL::~ShaderObjectOpenGL()
{
    glDeleteProgram(program);
    glDeleteShader(vertex_shader->GetID());
    glDeleteShader(pixel_shader->GetID());
}

}  // namespace Fuego::Renderer