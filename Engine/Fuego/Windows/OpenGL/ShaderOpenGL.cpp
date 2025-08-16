#include "ShaderOpenGL.h"

#include "ShaderObjectOpenGL.h"
#include "TextureOpenGL.h"
#include "glad/gl.h"

namespace Fuego::Graphics
{

GLint GetShaderType(Shader::ShaderType type)
{
    switch (type)
    {
    case Shader::Pixel:
        return GL_FRAGMENT_SHADER;
    case Shader::Vertex:
        return GL_VERTEX_SHADER;
    default:
        FU_CORE_ASSERT(false, "[Shader] Invalid shader type:")
        return 0;
    }
}

ShaderOpenGL::ShaderOpenGL(std::string_view name, const char* shaderCode, ShaderType type)
    : Shader(name)
    , shader_object(0)
    , _type(type)
{
    _shaderID = glCreateShader(GetShaderType(type));
    glShaderSource(_shaderID, 1, &shaderCode, nullptr);
    glCompileShader(_shaderID);

    std::string shader_type{};
    if (type == ShaderType::Vertex)
        shader_type = "vertex";
    else if (type == ShaderType::Pixel)
        shader_type = "pixel";

    GLint success;
    glGetShaderiv(_shaderID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(_shaderID, 512, nullptr, infoLog);
        FU_CORE_ERROR("[Shader] {0} {1} compilation error: ",name, shader_type, infoLog);
    }
    else
    {
        FU_CORE_TRACE("[Shader] {0} {1} has compiled", name, shader_type);
        glObjectLabel(GL_SHADER, _shaderID, -1, this->name.c_str());
    }
}

ShaderOpenGL::~ShaderOpenGL()
{
    Release();
}

void ShaderOpenGL::BindToShaderObject(ShaderObject& obj)
{
    ShaderObjectOpenGL& obj_gl = dynamic_cast<ShaderObjectOpenGL&>(obj);
    shader_object = obj_gl.GetObjectID();
}

void ShaderOpenGL::Release()
{
    glDeleteShader(_shaderID);

    shader_object = 0;
    _shaderID = 0;
    _type = None;
}

}  // namespace Fuego::Graphics
