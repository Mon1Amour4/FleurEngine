#pragma once

#include "glm/glm.hpp"

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

    virtual bool AddVar(const std::string& name) = 0;
    virtual bool SetVec3f(const std::string& var, glm::vec3 vector) = 0;
    virtual bool SetMat4f(const std::string& var, glm::mat4 matrix) = 0;

    virtual ~Shader() = default;
};
}  // namespace Fuego::Renderer
