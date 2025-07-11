#include "Image2D.h"

#include "Color.h"
#include "Services/ServiceLocator.h"

// Image2D:

Fuego::Graphics::CubemapImage Fuego::Graphics::Image2D::GenerateCubemapImage() const
{
    uint32_t face_size = width / 4;
    constexpr float pi = glm::pi<float>();

    std::array<Image2D, 6> out_faces;

    for (size_t face = 0; face < 6; face++)
    {
        Bitmap<BitmapFormat_UnsignedByte> new_bitmap(face_size, face_size, 3);

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

                glm::vec4 color = bitmap.GetPixel(u, v);
                new_bitmap.SetPixel(coord_u, coord_v, color);
            }
        }
        out_faces[face] = std::move(Image2D(name, ext, std::move(new_bitmap), face_size, face_size, channels, depth));
    }
    return CubemapImage(std::move(out_faces));
}


// Cubemap:
Fuego::Graphics::CubemapImage::CubemapImage(std::array<Image2D, 6>&& in_faces)
    : ImageBase(in_faces[0].Name(), "-", in_faces[0].Width(), in_faces[0].Width(), in_faces[0].Channels(), in_faces[0].Depth())

{
    size_bytes = width * height * channels * depth * 6;
    faces = std::move(in_faces);
}

const Fuego::Graphics::Image2D& Fuego::Graphics::CubemapImage::GetFace(Face face) const
{
    switch (face)
    {
    case Fuego::Graphics::CubemapImage::Face::Right:
        return faces[0];
    case Fuego::Graphics::CubemapImage::Face::Left:
        return faces[1];
    case Fuego::Graphics::CubemapImage::Face::Top:
        return faces[2];
    case Fuego::Graphics::CubemapImage::Face::Bottom:
        return faces[3];
    case Fuego::Graphics::CubemapImage::Face::Back:
        return faces[4];
    case Fuego::Graphics::CubemapImage::Face::Front:
        return faces[5];
    }
}