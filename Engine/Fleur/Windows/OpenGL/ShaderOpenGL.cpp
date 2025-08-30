#include "ShaderOpenGL.h"

#include "ShaderObjectOpenGL.h"
#include "TextureOpenGL.h"
#include "glad/gl.h"

namespace Fleur::Graphics
{

GLint GetShaderType(Shader::EShaderType type)
{
    switch (type)
    {
    case Shader::Pixel:
        return GL_FRAGMENT_SHADER;
    case Shader::Vertex:
        return GL_VERTEX_SHADER;
    default:
        FL_CORE_ASSERT(false, "[Shader] Invalid shader type:")
        return 0;
    }
}

ShaderOpenGL::ShaderOpenGL(std::string_view name, const char* shaderCode, EShaderType type)
    : Shader(name)
    , m_ShaderObject(0)
    , m_Type(type)
{
    m_ShaderID = glCreateShader(GetShaderType(type));
    glShaderSource(m_ShaderID, 1, &shaderCode, nullptr);
    glCompileShader(m_ShaderID);

    std::string shaderType{};
    if (type == EShaderType::Vertex)
        shaderType = "vertex";
    else if (type == EShaderType::Pixel)
        shaderType = "pixel";

    GLint success;
    glGetShaderiv(m_ShaderID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(m_ShaderID, 512, nullptr, infoLog);
        FL_CORE_ERROR("[Shader] {0} {1} compilation error: ",name, shaderType, infoLog);
    }
    else
    {
        FL_CORE_TRACE("[Shader] {0} {1} has compiled", name, shaderType);
        glObjectLabel(GL_SHADER, m_ShaderID, -1, this->m_Name.c_str());
    }
}

ShaderOpenGL::~ShaderOpenGL()
{
    Release();
}

void ShaderOpenGL::BindToShaderObject(ShaderObject& obj)
{
    ShaderObjectOpenGL& objectGL = static_cast<ShaderObjectOpenGL&>(obj);
    m_ShaderObject = objectGL.GetObjectID();
}

void ShaderOpenGL::Release()
{
    glDeleteShader(m_ShaderID);

    m_ShaderObject = 0;
    m_ShaderID = 0;
    m_Type = None;
}

}  // namespace Fleur::Graphics
