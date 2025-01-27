#pragma once

#include <cstdint>
#include <glm/fwd.hpp>

#include "Renderer/Shader.h"
#include "glad/gl.h"

namespace Fuego::Renderer
{
class ShaderOpenGL : public Shader
{
public:
    virtual ~ShaderOpenGL() override;

    inline ShaderType GetType() const
    {
        return _type;
    }
    inline uint32_t GetID() const
    {
        return _shaderID;
    }

    virtual void BindToShaderObject(ShaderObject& obj) override;
    virtual bool AddVar(const std::string& uniform) override;
    virtual bool SetVec3f(const std::string& var, glm::vec3 vector) override;
    virtual bool SetMat4f(const std::string& var, glm::mat4 matrix) override;

private:
    uint16_t shader_object;
    uint32_t _shaderID;
    ShaderType _type;

    GLint GetShaderType(ShaderType type) const;

    std::unordered_map<std::string, uint16_t> uniforms;

protected:
    friend class DeviceOpenGL;
    ShaderOpenGL(const char* shaderCode, ShaderType type);
};
}  // namespace Fuego::Renderer
