#pragma once

#include "Renderer/Shader.h"

namespace Fleur::Graphics
{
class ShaderOpenGL final : public Shader
{
public:
    //FLEUR_NON_COPYABLE_NON_MOVABLE(ShaderOpenGL)

    virtual ~ShaderOpenGL() override;

    inline EShaderType GetType() const
    {
        return m_Type;
    }
    inline uint32_t GetID() const
    {
        return m_ShaderID;
    }

    virtual void BindToShaderObject(ShaderObject& obj) override;

    virtual void Release() override;

private:
    uint32_t m_ShaderObject;
    uint32_t m_ShaderID;
    EShaderType m_Type;

protected:
    friend class DeviceOpenGL;
    ShaderOpenGL(std::string_view name, const char* shaderCode, EShaderType type);
};
}  // namespace Fleur::Graphics
