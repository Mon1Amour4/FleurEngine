#pragma once

#include "ShaderObject.h"
#include "ShaderOpenGl.h"

namespace Fleur::Graphics
{
class ShaderObjectOpenGL : public ShaderObject
{
public:
    friend class ShaderObject;

    virtual ~ShaderObjectOpenGL() override;

    inline virtual uint32_t GetObjectID()
    {
        return m_Program;
    }

    virtual void Use() const override;
    bool AddVar(std::string_view uniformName, uint32_t id);

    virtual void BindMaterial(const Material* material) override;

    virtual void Release() override;

private:
    const Material* m_Material;
    uint32_t m_Program;

    std::unique_ptr<ShaderOpenGL> m_VertexShader;
    std::unique_ptr<ShaderOpenGL> m_PixelShader;

    ShaderObjectOpenGL(std::string_view name, Shader* vs, Shader* px);

private:
    std::unordered_map<std::string, uint32_t, std::hash<std::string>, std::equal_to<std::string>>
        m_Uniforms;

    int find_uniform_location(std::string_view uniformName) const;

protected:
    virtual bool SetVec3fImpl(std::string_view uniformName, const glm::vec3& vec) override;
    virtual bool SetMat4fImpl(std::string_view uniformName, const glm::mat4& matrix) override;
    virtual bool SetText2dImpl(std::string_view uniformName, const Texture& texture) override;
};
}  // namespace Fleur::Graphics
