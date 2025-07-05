#include "CubemapOpenGL.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "../../../../CoreLib/Bitmap.hpp"
#include "glad/wgl.h"

Fuego::Graphics::CubemapOpenGL::CubemapOpenGL(const Image2D* img_1, const Image2D* img_2, const Image2D* img_3, const Image2D* img_4, const Image2D* img_5,
                                              const Image2D* img_6)
    : Cubemap(img_1, img_2, img_3, img_4, img_5, img_6)
{
    FU_CORE_ASSERT(img_1->Width() == img_2->Width() == img_3->Width() == img_4->Width() == img_5->Width() == img_6->Width(), "");
    FU_CORE_ASSERT(img_1->Height() == img_2->Height() == img_3->Height() == img_4->Height() == img_5->Height() == img_6->Height(), "");
    FU_CORE_ASSERT(img_1->Width() == img_1->Height(), "");

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
    glTextureStorage2D(id, 1, GL_RGB8, img_1->Width(), img_1->Height());

    std::vector<const Image2D*> faces{img_1, img_2, img_3, img_4, img_5, img_6};
    FU_CORE_ASSERT(faces.size() != 6, "");

    for (int i = 0; i < faces.size(); ++i)
    {
        glTextureSubImage3D(id,
                            0,                   // mipmap level
                            0,                   // xoffset
                            0,                   // yoffset
                            i,                   // zoffset = face index
                            faces[i]->Width(),   // width
                            faces[i]->Height(),  // height
                            1,                   // depth = 1
                            GL_RGB,              // format
                            GL_UNSIGNED_BYTE,
                            reinterpret_cast<const void*>(faces[i]->Data())  // pointer to data
        );
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

Fuego::Graphics::CubemapOpenGL::CubemapOpenGL(const Image2D* equirectangular)
    : Cubemap(equirectangular)
{
    FU_CORE_ASSERT(equirectangular->Width() / equirectangular->Height() == 2.0f, "");

    Fuego::Bitmap<BitmapFormat_UnsignedByte> equirectangular_bitmap(equirectangular);

    uint32_t face_size = equirectangular->Width() / 4.f;
    constexpr float pi = glm::pi<float>();

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
    glTextureStorage2D(id, 1, GL_RGB8, face_size, face_size);

    for (size_t face = 0; face < 6; face++)
    {
        Fuego::Bitmap<BitmapFormat_UnsignedByte> bitmap(face_size, face_size, equirectangular->Channels());

        for (size_t coord_u = 0; coord_u < face_size; coord_u++)
        {
            for (size_t coord_v = 0; coord_v < face_size; coord_v++)
            {
                float normalized_u = (2.f * coord_u + 1) / face_size - 1.f;
                float normalized_v = (2.f * coord_v + 1) / face_size - 1.f;

                glm::vec3 direction;
                switch (face)
                {
                case 0:
                    direction = glm::vec3(1.f, normalized_v, -normalized_u);
                    break;
                case 1:
                    direction = glm::vec3(-1.f, normalized_v, normalized_u);
                    break;
                case 2:
                    direction = glm::vec3(normalized_u, 1.f, normalized_v);
                    break;
                case 3:
                    direction = glm::vec3(normalized_u, -1.f, -normalized_v);
                    break;
                case 4:
                    direction = glm::vec3(normalized_u, normalized_v, 1.f);
                    break;
                case 5:
                    direction = glm::vec3(-normalized_u, normalized_v, -1.f);
                    break;
                }

                glm::vec3 spherical_dir = glm::normalize(glm::vec3(-direction.z, direction.x, direction.y));
                float radius = glm::length(spherical_dir);
                float phi = atan2(spherical_dir.y, spherical_dir.x);
                float theta = atan2(spherical_dir.z, radius);


                float u = static_cast<float>((phi + pi) / (2.f * pi));
                float v = static_cast<float>((pi / 2.f - theta) / pi);

                glm::vec4 color = equirectangular_bitmap.GetPixel(u, v);
                bitmap.SetPixel(coord_u, coord_v, color);
            }
        }
        glTextureSubImage3D(id,
                            0,          // mipmap level
                            0,          // xoffset
                            0,          // yoffset
                            face,       // zoffset = face index
                            face_size,  // width
                            face_size,  // height
                            1,          // depth = 1
                            GL_RGB,     // format
                            GL_UNSIGNED_BYTE,
                            reinterpret_cast<const void*>(bitmap.Data())  // pointer to data
        );
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}