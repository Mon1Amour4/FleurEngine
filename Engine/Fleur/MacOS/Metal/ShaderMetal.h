#pragma once

#include <Metal/Metal.hpp>

#include "Lux/Shader.h"

namespace Fleur::Renderer
{
class ShaderMetal : public Shader
{
public:
    friend class CommandBufferMetal;

    ShaderMetal(MTL::Function* function);
    virtual void BindToShaderObject(ShaderObject& obj) override
    {
        UNUSED(obj);
    }
    virtual bool AddVar(const std::string& name) override
    {
        return false;
    }
    virtual bool SetVec3f(const std::string& var, Fleur::Math::vec3 vector) const override
    {
        return false;
    }
    virtual bool SetMat4f(const std::string& var, Fleur::Math::mat4 matrix) const override
    {
        return false;
    }
    virtual bool SetText2D(const std::string& var, const Texture& texture) const override
    {
        return false;
    }

private:
    MTL::Function* _function;
};
}  // namespace Fleur::Renderer
