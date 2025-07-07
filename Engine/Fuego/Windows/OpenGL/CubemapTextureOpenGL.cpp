#include "CubemapTextureOpenGL.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "../../../../CoreLib/Bitmap.hpp"
#include "glad/wgl.h"

Fuego::Graphics::CubemapTextureOpenGL::CubemapTextureOpenGL(const Fuego::Graphics::CubemapImage* img)
    : CubemapTexture(img)
    , TextureOpenGL("asd")
{
    // Fuego::Bitmap<BitmapFormat_UnsignedByte> equirectangular_bitmap(equirectangular->Data(), equirectangular->Width(), equirectangular->Height(),
    // equirectangular->Channels());
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
    glTextureStorage2D(id, 1, GL_RGB8, img->FaceSize(), img->FaceSize());

    // glTextureSubImage3D(id,
    //                     0,          // mipmap level
    //                     0,          // xoffset
    //                     0,          // yoffset
    //                     face,       // zoffset = face index
    //                     face_size,  // width
    //                     face_size,  // height
    //                     1,          // depth = 1
    //                     GL_RGB,     // format
    //                     GL_UNSIGNED_BYTE,
    //                     reinterpret_cast<const void*>(bitmap.Data())  // pointer to data
    //);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}