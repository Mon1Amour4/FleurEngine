#include "Image2D.h"

#include "Color.h"
#include "Image2D.h"
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

Fuego::Graphics::ImageBase::ImageBase(ImageBase&& other) noexcept
    : name(std::move(other.name))
    , extension(std::move(other.extension))
    , width(other.width)
    , height(other.height)
    , channels(other.channels)
    , depth(other.depth)
    , layers(other.layers)
    , size_bytes(other.size_bytes)
    , is_created(other.is_created)
{
    other.width = 0;
    other.height = 0;
    other.channels = 0;
    other.depth = 0;
    other.layers = 0;
    other.size_bytes = 0;
    other.is_created = false;
}
Fuego::Graphics::ImageBase& Fuego::Graphics::ImageBase::operator=(ImageBase&& other) noexcept
{
    if (this != &other)
    {
        name = std::move(other.name);
        extension = std::move(other.extension);
        width = other.width;
        height = other.height;
        channels = other.channels;
        depth = other.depth;
        layers = other.layers;
        size_bytes = other.size_bytes;
        is_created = other.is_created;

        other.width = 0;
        other.height = 0;
        other.channels = 0;
        other.depth = 0;
        other.layers = 0;
        other.size_bytes = 0;
        other.is_created = false;
    }
    return *this;
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

Fuego::Graphics::Image2D Fuego::Graphics::Image2D::FromEquirectangularToCross() const
{
    uint32_t face_size = width / 4;
    constexpr float pi = glm::pi<float>();

    uint32_t stride = width * channels;
    Bitmap<BitmapFormat_UnsignedByte> out_bitmap(face_size * 4, face_size * 3, channels);
    struct FacePos
    {
        int x, y;
    };
    std::map<int, FacePos> facePos = {
        {6, {2, 1}},  // POSITIVE_X
        {4, {0, 1}},  // NEGATIVE_X
        {1, {1, 0}},  // POSITIVE_Y
        {9, {1, 2}},  // NEGATIVE_Y
        {7, {1, 1}},  // POSITIVE_Z
        {5, {3, 1}}   // NEGATIVE_Z
    };
    for (size_t face = 0; face < 12; face++)
    {
        if (face == 0 || face == 2 || face == 3 || face == 8 || face == 10 || face == 11)
        {
            continue;
        }
        for (size_t coord_u = 0; coord_u < face_size; coord_u++)
        {
            for (size_t coord_v = 0; coord_v < face_size; coord_v++)
            {
                float normalized_u = (2.f * coord_u + 1) / face_size - 1.f;
                float normalized_v = (2.f * coord_v + 1) / face_size - 1.f;

                glm::vec3 direction;
                switch (face)
                {
                case 6:
                    direction = glm::vec3(1.f, normalized_v, -normalized_u);  // POSITIVE_X
                    break;
                case 4:
                    direction = glm::vec3(-1.f, normalized_v, normalized_u);  // NEGATIVE_X
                    break;
                case 1:
                    direction = glm::vec3(normalized_u, -1.f, -normalized_v);  // POSITIVE_Y
                    break;
                case 9:
                    direction = glm::vec3(normalized_u, 1.f, normalized_v);  // NEGATIVE_Y
                    break;
                case 7:
                    direction = glm::vec3(normalized_u, normalized_v, 1.f);  // POSITIVE_Z
                    break;
                case 5:
                    direction = glm::vec3(-normalized_u, normalized_v, -1.f);  // NEGATIVE_Z
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
                int out_x = facePos[face].x * face_size + coord_u;
                int out_y = facePos[face].y * face_size + coord_v;
                out_bitmap.SetPixel(out_x, out_y, color);
            }
        }
    }
    return Image2D(name + "_cross_layout", extension, reinterpret_cast<unsigned char*>(out_bitmap.Data()), face_size * 4, face_size * 3, channels, depth);
}

Fuego::Graphics::CubemapImage Fuego::Graphics::Image2D::FromCrossToCubemap() const
{
    std::array<Image2D, 6> out_faces;

    uint32_t face_size = width / 4;
    auto get_stride = [this](TextureFormat format)
    {
        switch (format)
        {
        case TextureFormat::RGB8:
            return 3;
        case TextureFormat::RGBA8:
            return 4;
        }
    };
    uint32_t stride = get_stride(Texture::GetTextureFormat(channels, depth));

    auto upload_face = [this](std::array<Image2D, 6>& array, const unsigned char* buffer, uint32_t start_x, uint32_t start_y, uint32_t face_size,
                              uint32_t width, uint32_t stride, uint32_t in_face, uint32_t out_face)
    {
        for (size_t i = 0; i < face_size; i++)
        {
            // array[out_face].bitmap.SetPixel()
        }
        uint32_t row_stride = width * stride;
        const uint8_t* base = buffer + (start_y * width + start_x) * stride;
    };
    // upload_face(out_faces, reinterpret_cast<const unsigned char*>(bitmap.Data()), face_size, 0, face_size, width, stride, 2, 0);              // top
    // upload_face(out_faces, reinterpret_cast<const unsigned char*>(bitmap.Data()), 0, face_size, face_size, width, stride, 1, 1);              // left
    // upload_face(out_faces, reinterpret_cast<const unsigned char*>(bitmap.Data()), face_size, face_size, face_size, width, stride, 4, 2);      // front
    // upload_face(out_faces, reinterpret_cast<const unsigned char*>(bitmap.Data()), face_size * 2, face_size, face_size, width, stride, 0, 3);  // right
    // upload_face(out_faces, reinterpret_cast<const unsigned char*>(bitmap.Data()), face_size * 3, face_size, face_size, width, stride, 5, 4);  // back
    // upload_face(out_faces, reinterpret_cast<const unsigned char*>(bitmap.Data()), face_size, face_size * 2, face_size, width, stride, 3, 5);  // bottom

    return Fuego::Graphics::CubemapImage("", "");
}


// CubemapImage:
Fuego::Graphics::CubemapImage::CubemapImage(std::array<Image2D, 6>&& in_faces)
    : ImageBase(in_faces[0].Name(), in_faces[0].Ext(), in_faces[0].Width(), in_faces[0].Width(), in_faces[0].Channels(), in_faces[0].Depth(), 6)

{
    faces = std::move(in_faces);
}
Fuego::Graphics::CubemapImage::CubemapImage(std::string_view name, std::string_view ext)
    : ImageBase(name, ext, 6)
{
}

Fuego::Graphics::CubemapImage& Fuego::Graphics::CubemapImage::operator=(CubemapImage&& other) noexcept
{
    if (this != &other)
    {
        ImageBase::operator=(std::move(other));
        faces = std::move(other.faces);
    }
    return *this;
}
Fuego::Graphics::CubemapImage::CubemapImage(CubemapImage&& other) noexcept
    : ImageBase(std::move(other))
    , faces(std::move(other.faces))
{
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
    // faces = reinterpret_cast<const std::array<Fuego::Graphics::Image2D, 6>*>(settings.data);
    //  uint32_t data_chank_size = settings.width * settings.height * settings.channels * settings.depth*;
    /* for (size_t i = 0; i < layers; i++)
     {
         Bitmap<BitmapFormat_UnsignedByte> bitmap = Bitmap<BitmapFormat_UnsignedByte>(settings.width, settings.height, settings.channels);
         faces[i] = Image2D(name, extension, std::move(bitmap), settings.width, settings.height, settings.channels, settings.depth);
     }

     memcpy_s(bitmap.Data(), bitmap.GetSizeBytes(), settings.data, settings.width * settings.height * settings.channels * settings.depth);*/

    is_created = true;
}
