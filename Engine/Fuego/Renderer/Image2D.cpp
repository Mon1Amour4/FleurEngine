#include "Image2D.h"

#include "Color.h"
#include "Services/ServiceLocator.h"

// ImageBase
Fuego::Graphics::ImageBase::ImageBase(std::string_view name, std::string_view ext, uint32_t layers)
    : name(name)
    , extension(ext)
    , width(0)
    , height(0)
    , channels(0)
    , depth(0)
    , layers(layers)
    , size_bytes(0)
    , is_created(false)
{
}
Fuego::Graphics::ImageBase::ImageBase(std::string_view name, std::string_view ext, uint32_t width, uint32_t height, uint16_t channels, uint16_t depth,
                                      uint32_t layers)
    : name(name)
    , extension(ext)
    , width(width)
    , height(height)
    , channels(channels)
    , depth(depth)
    , layers(layers)
    , size_bytes(width * height * channels * depth * layers)
    , is_created(false)
{
}

// Image2D:
Fuego::Graphics::Image2D::Image2D()
    : ImageBase()
{
}
Fuego::Graphics::Image2D::Image2D(std::string_view name, std::string_view ext)
    : ImageBase(name, ext, 1)
{
}
Fuego::Graphics::Image2D::Image2D(std::string_view name, std::string_view ext, unsigned char* data, int w, int h, uint16_t channels, uint16_t depth)
    : ImageBase(name, ext, w, h, channels, depth, 1)
    , bitmap(w, h, channels)
{
    FU_CORE_ASSERT(depth > 0 && channels > 0, "Invalid Image data");
    memcpy(bitmap.Data(), data, bitmap.GetSizeBytes());
    is_created = true;
}
Fuego::Graphics::Image2D::Image2D(std::string_view name, std::string_view ext, Bitmap<BitmapFormat_UnsignedByte>&& in_bitmap, int w, int h, uint16_t channels,
                                  uint16_t depth)
    : ImageBase(name, ext, w, h, channels, depth, 1)
    , bitmap(std::move(in_bitmap))
{
    is_created = true;
};

Fuego::Graphics::Image2D& Fuego::Graphics::Image2D::operator=(Fuego::Graphics::Image2D&& other) noexcept
{
    if (this != &other)
    {
        bitmap = std::move(other.bitmap);
        width = other.width;
        height = other.height;
        is_created = other.is_created;
        name = std::move(other.name);
        extension = std::move(other.extension);
        channels = other.channels;
        depth = other.depth;
        layers = other.layers;

        other.width = 0;
        other.height = 0;
        other.is_created = false;
        other.channels = 0;
        other.depth = 0;
        other.layers = 0;
    }
    return *this;
}
Fuego::Graphics::Image2D::Image2D(Fuego::Graphics::Image2D&& other) noexcept
{
    bitmap = std::move(other.bitmap);
    width = other.width;
    height = other.height;
    is_created = other.is_created;
    name = std::move(other.name);
    extension = std::move(other.extension);
    channels = other.channels;
    depth = other.depth;
    layers = other.layers;

    other.width = 0;
    other.height = 0;
    other.is_created = false;
    other.channels = 0;
    other.depth = 0;
    other.layers = 0;
}

const void* Fuego::Graphics::Image2D::Data() const
{
    return bitmap.Data();
}

void Fuego::Graphics::Image2D::PostCreate(ImagePostCreation& settings)
{
    FU_CORE_ASSERT(settings.data, "[Image2D] data is nullptr");

    ImageBase::PostCreate(settings);

    bitmap = Bitmap<BitmapFormat_UnsignedByte>(settings.width, settings.height, settings.channels);
    memcpy_s(bitmap.Data(), bitmap.GetSizeBytes(), settings.data, settings.width * settings.height * settings.channels * settings.depth);

    is_created = true;
}

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

                float U = u * width;
                float V = v * height;

                int MaxW = width - 1;
                int MaxH = height - 1;
                float U1 = std::clamp(int(roundf(U)), 0, MaxW);
                float V1 = std::clamp(int(roundf(V)), 0, MaxH);

                glm::vec4 color = bitmap.GetPixel(U1, V1);
                new_bitmap.SetPixel(coord_u, coord_v, color);
            }
        }
        out_faces[face] = std::move(Image2D(name, extension, std::move(new_bitmap), face_size, face_size, channels, depth));
    }
    return CubemapImage(std::move(out_faces));
}


// CubemapImage:
Fuego::Graphics::CubemapImage::CubemapImage(std::array<Image2D, 6>&& in_faces)
    : ImageBase(in_faces[0].Name(), in_faces[0].Ext(), in_faces[0].Width(), in_faces[0].Width(), in_faces[0].Channels(), in_faces[0].Depth(), 6)

{
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

const Fuego::Graphics::Image2D* Fuego::Graphics::CubemapImage::Data() const
{
    return reinterpret_cast<const Image2D*>(faces.data());
}

void Fuego::Graphics::CubemapImage::PostCreate(ImagePostCreation& settings)
{
    FU_CORE_ASSERT(settings.data, "[Image2D] data is nullptr");

    ImageBase::PostCreate(settings);

    // uint32_t data_chank_size = settings.width * settings.height * settings.channels * settings.depth*;
    /* for (size_t i = 0; i < layers; i++)
     {
         Bitmap<BitmapFormat_UnsignedByte> bitmap = Bitmap<BitmapFormat_UnsignedByte>(settings.width, settings.height, settings.channels);
         faces[i] = Image2D(name, extension, std::move(bitmap), settings.width, settings.height, settings.channels, settings.depth);
     }

     memcpy_s(bitmap.Data(), bitmap.GetSizeBytes(), settings.data, settings.width * settings.height * settings.channels * settings.depth);*/

    is_created = true;
}
