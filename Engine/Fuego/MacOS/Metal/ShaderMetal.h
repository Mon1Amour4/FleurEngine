#pragma once

#include <Metal/Metal.hpp>

#include "Renderer/Shader.h"

namespace Fuego::Renderer
{
class ShaderMetal : public Shader
{
public:
    friend class CommandBufferMetal;

    ShaderMetal(MTL::Function* function);

private:
    MTL::Function* _function;
};
}  // namespace Fuego::Renderer
