#pragma once

#include "glm/common.hpp"

namespace Fuego::Renderer
{
class Shader
{
public:
    enum ShaderType
    {
        Vertex = 1,
        Pixel = 2
    };
    virtual bool AddVar(const std::string& name);
    virtual bool SetVec3f(glm::vec3 vector);

    virtual ~Shader() = default;
};
}  // namespace Fuego::Renderer
