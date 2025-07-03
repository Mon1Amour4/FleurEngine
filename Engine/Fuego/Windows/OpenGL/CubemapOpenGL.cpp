#include "CubemapOpenGL.h"

#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

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

    float face_size = equirectangular->Width() / 4.f;
    for (size_t face = 0; face < 6; face++)
    {
        for (size_t coord_u = 0; coord_u < face_size; coord_u++)
        {
            for (size_t coord_v = 0; coord_v < face_size; coord_v++)
            {
                float normalized_u = (2.f * coord_u + 1) / face_size - 1.f;
                float normalized_v = (2.f * coord_v + 1) / face_size - 1.f;

                glm::vec3 direction;
                switch (face)
                {
                case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
                    direction = glm::vec3(1.f, normalized_v, -normalized_u);
                    break;
                case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
                    direction = glm::vec3(-1.f, normalized_v, normalized_u);
                    break;
                case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
                    direction = glm::vec3(normalized_u, 1.f, normalized_v);
                    break;
                case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
                    direction = glm::vec3(normalized_u, -1.f, -normalized_v);
                    break;
                case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
                    direction = glm::vec3(normalized_u, normalized_v, 1.f);
                    break;
                case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                    direction = glm::vec3(-normalized_u, normalized_v, -1.f);
                    break;
                }

                glm::vec3 radius = glm::normalize(glm::vec3(-direction.z, direction.x, direction.y));
                float theta = acos(std::clamp(radius.z, -1.0f, 1.0f));
                float phi = atan2(radius.y, radius.x);
            }
        }
    }
}