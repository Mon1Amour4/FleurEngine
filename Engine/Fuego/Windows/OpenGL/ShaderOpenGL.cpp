#include "ShaderOpenGL.h"

#include <glm/gtc/type_ptr.hpp>

#include "ShaderObjectOpenGL.h"

namespace Fuego::Renderer
{
ShaderOpenGL::ShaderOpenGL(const char* shaderCode, ShaderType type)
    : _type(type)
{
    _shaderID = glCreateShader(GetShaderType(type));
    glShaderSource(_shaderID, 1, &shaderCode, nullptr);
    glCompileShader(_shaderID);

    GLint success;
    glGetShaderiv(_shaderID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(_shaderID, 512, nullptr, infoLog);
        FU_CORE_ERROR("[Shader] compilation error: ", infoLog);
    }
    else
    {
        FU_CORE_TRACE("[Shader] has compiled");
    }
}

ShaderOpenGL::~ShaderOpenGL()
{
    glDeleteShader(_shaderID);
}

GLint ShaderOpenGL::GetShaderType(ShaderType type) const
{
    switch (type)
    {
    case Pixel:
        return GL_FRAGMENT_SHADER;
    case Vertex:
        return GL_VERTEX_SHADER;
    default:
        FU_CORE_ERROR("[Shader] Invalid shader type:");
        break;
    }
}

void ShaderOpenGL::BindToShaderObject(ShaderObject& obj)
{
    ShaderObjectOpenGL& obj_gl = static_cast<ShaderObjectOpenGL&>(obj);
    shader_object = obj_gl.GetObjectID();
}

bool ShaderOpenGL::AddVar(const std::string& uniform)
{
    GLint location = glGetUniformLocation(shader_object, uniform.c_str());
    if (location != -1)
    {
        uniforms[uniform] = location;
        return true;
    }
    else
    {
        FU_CORE_ERROR("[Shader] Uniform {} not found in shader", uniform.c_str());
        return false;
    }
}

bool ShaderOpenGL::SetVec3f(const std::string& var, glm::vec3 vector)
{
    auto it = uniforms.find(var);
    if (it != uniforms.end())
    {
        glUniform3f(it->second, vector.x, vector.y, vector.z);
        return true;
    }
    else
    {
        return false;
    }
}

bool ShaderOpenGL::SetMat4f(const std::string& var, glm::mat4 matrix)
{
    auto it = uniforms.find(var);
    if (it != uniforms.end())
    {
        glUniformMatrix4fv(it->second, 1, GL_FALSE, glm::value_ptr(matrix));

        return true;
    }
    else
    {
        return false;
    }
}

}  // namespace Fuego::Renderer
