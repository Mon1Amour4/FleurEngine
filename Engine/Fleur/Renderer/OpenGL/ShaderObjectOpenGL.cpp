#include "ShaderObjectOpenGL.h"

#include <glad/wgl.h>

#include "Renderer/Material.h"
#include "TextureOpenGL.h"

struct uniform_info
{
    GLint Location;
    GLsizei Count;
    GLenum Type;
};

Fleur::Graphics::ShaderObject* Fleur::Graphics::ShaderObject::CreateShaderObject(std::string_view name, Shader* vs, Shader* px)
{
    return new ShaderObjectOpenGL(name, vs, px);
}

Fleur::Graphics::ShaderObjectOpenGL::ShaderObjectOpenGL(std::string_view name, Shader* vs, Shader* px)
    : ShaderObject(name)
    , m_Program(glCreateProgram())
    , m_VertexShader(nullptr)
    , m_PixelShader(nullptr)
    , m_Material(nullptr)
{
    m_VertexShader.reset(static_cast<ShaderOpenGL*>(vs));
    m_PixelShader.reset(static_cast<ShaderOpenGL*>(px));

    glAttachShader(m_Program, m_VertexShader->GetID());
    glAttachShader(m_Program, m_PixelShader->GetID());

    glLinkProgram(m_Program);
    GLint success;
    glGetProgramiv(m_Program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(m_Program, 512, nullptr, infoLog);
      //  FL_CORE_ERROR("[ShaderObject] program linking error: ", infoLog);
    }

    GLint uniformCount = 0;
    glGetProgramiv(m_Program, GL_ACTIVE_UNIFORMS, &uniformCount);
    if (uniformCount > 0)
    {
        GLint max_name_len = 0;
        glGetProgramiv(m_Program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_len);

        auto uniform_name = std::make_unique<char[]>(max_name_len);

        for (GLint i = 0; i < uniformCount; ++i)
        {
            GLsizei length = 0;
            uniform_info info = {};
            glGetActiveUniform(m_Program, i, max_name_len, &length, &info.Count, &info.Type, uniform_name.get());
            info.Location = glGetUniformLocation(m_Program, uniform_name.get());

            AddVar(std::string(uniform_name.get(), length), info.Location);
        }
    }
    m_VertexShader->BindToShaderObject(*this);
    m_PixelShader->BindToShaderObject(*this);
    glObjectLabel(GL_PROGRAM, m_Program, -1, this->name.c_str());
    glUseProgram(0);
}

Fleur::Graphics::ShaderObjectOpenGL::~ShaderObjectOpenGL()
{
    Release();
}

void Fleur::Graphics::ShaderObjectOpenGL::Release()
{
    glDeleteProgram(m_Program);
    m_Program = 0;

    m_Material = nullptr;

    if (m_VertexShader.get())
    {
        m_VertexShader->Release();
        m_VertexShader.reset();
    }
    if (m_PixelShader.get())
    {
        m_PixelShader->Release();
        m_PixelShader.reset();
    }
    m_Uniforms.clear();
}

void Fleur::Graphics::ShaderObjectOpenGL::Use() const
{
    glUseProgram(m_Program);
}

void Fleur::Graphics::ShaderObjectOpenGL::BindMaterial(const Material* material)
{
    this->m_Material = material;
    const ShaderComponentContext& ctx = this->m_Material->GetShaderContext();
    if (ctx.albedo_text.second)
        SetText2dImpl(ctx.albedo_text.first, *ctx.albedo_text.second);
    if (ctx.skybox_cubemap_text.second)
        SetText2dImpl(ctx.skybox_cubemap_text.first, *ctx.skybox_cubemap_text.second);
}

int Fleur::Graphics::ShaderObjectOpenGL::find_uniform_location(std::string_view uniform_name) const
{
    if (uniform_name.empty())
        return -1;

    auto it = m_Uniforms.find(uniform_name.data());
    return (it != m_Uniforms.end()) ? it->second : -1;
}

bool Fleur::Graphics::ShaderObjectOpenGL::AddVar(std::string_view uniformName, uint32_t id)
{
    GLint location = find_uniform_location(uniformName);
    if (location != -1)
        return true;

    m_Uniforms.emplace(uniformName.data(), id);
    return true;
}

bool Fleur::Graphics::ShaderObjectOpenGL::SetVec3fImpl(std::string_view uniformName, const glm::vec3& vec)
{
    GLint location = find_uniform_location(uniformName);
    if (location == -1)
        return false;

    glUniform3f(location, vec.x, vec.y, vec.z);
    return true;
}

bool Fleur::Graphics::ShaderObjectOpenGL::SetMat4fImpl(std::string_view uniformName, const glm::mat4& matrix)
{
    GLint location = find_uniform_location(uniformName);
    if (location == -1)
        return false;

    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    return true;
}

bool Fleur::Graphics::ShaderObjectOpenGL::SetText2dImpl(std::string_view uniformName, const Texture& texture)
{
    GLint location = find_uniform_location(uniformName);
    if (location == -1)
    {
        //FL_CORE_ASSERT(false, "");
        return false;
    }

    const TextureOpenGL& textureGL = static_cast<const TextureOpenGL&>(texture);
    glUniform1i(location, textureGL.GetTextureUnit());
    glBindTextureUnit(textureGL.GetTextureUnit(), *textureGL.GetTextureID());
    return true;
}
