#pragma once

#include "Renderer/Shader.h"

namespace Fleur::Graphics
{
class ShaderOpenGL final : public Shader
{
public:
    FLEUR_NON_COPYABLE_NON_MOVABLE(ShaderOpenGL)

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

    virtual void Release() override;

private:
    uint32_t shader_object;
    uint32_t _shaderID;
    ShaderType _type;

protected:
    friend class DeviceOpenGL;
    ShaderOpenGL(std::string_view name, const char* shaderCode, ShaderType type);
};
}  // namespace Fleur::Graphics
