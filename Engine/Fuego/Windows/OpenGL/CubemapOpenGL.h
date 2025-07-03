#pragma once

#include "../../Renderer/Cubemap.h"

namespace Fuego::Graphics
{
class CubemapOpenGL : public Cubemap
{
    CubemapOpenGL(const Image2D* img_1, const Image2D* img_2, const Image2D* img_3, const Image2D* img_4, const Image2D* img_5, const Image2D* img_6);
};

}  // namespace Fuego::Graphics