#include "Image2D.h"

#include "Color.h"
#include "Services/ServiceLocator.h"


// ImageBase
Fleur::Graphics::ImageBase::ImageBase(std::string_view name, std::string_view ext, uint32_t layers)
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
Fleur::Graphics::ImageBase::ImageBase(std::string_view name, std::string_view ext, uint32_t width, uint32_t height, uint16_t channels, uint16_t depth,
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

Fleur::Graphics::ImageBase::ImageBase(ImageBase&& other) noexcept
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
Fleur::Graphics::ImageBase& Fleur::Graphics::ImageBase::operator=(ImageBase&& other) noexcept
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
Fleur::Graphics::Image2D::Image2D()
    : ImageBase()
{
}
Fleur::Graphics::Image2D::Image2D(std::string_view name, std::string_view ext)
    : ImageBase(name, ext, 1)
{
}
Fleur::Graphics::Image2D::Image2D(std::string_view name, std::string_view ext, int w, int h, uint16_t channels, uint16_t depth)
    : ImageBase(name, ext, w, h, channels, depth, 1)
{
}
Fleur::Graphics::Image2D::Image2D(std::string_view name, std::string_view ext, unsigned char* data, int w, int h, uint16_t channels, uint16_t depth)
    : ImageBase(name, ext, w, h, channels, depth, 1)
    , bitmap(w, h, channels)
{
    FL_CORE_ASSERT(depth > 0 && channels > 0, "Invalid Image data");
    memcpy(bitmap.Data(), data, bitmap.GetSizeBytes());
    is_created = true;
}
Fleur::Graphics::Image2D::Image2D(std::string_view name, std::string_view ext, Bitmap<BitmapFormat_UnsignedByte>&& in_bitmap, int w, int h, uint16_t channels,
                                  uint16_t depth)
    : ImageBase(name, ext, w, h, channels, depth, 1)
    , bitmap(std::move(in_bitmap))
{
    is_created = true;
};

Fleur::Graphics::Image2D& Fleur::Graphics::Image2D::operator=(Fleur::Graphics::Image2D&& other) noexcept
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
        size_bytes = other.size_bytes;

        other.width = 0;
        other.height = 0;
        other.is_created = false;
        other.channels = 0;
        other.depth = 0;
        other.layers = 0;
        other.size_bytes = 0;
    }
    return *this;
}
Fleur::Graphics::Image2D::Image2D(Fleur::Graphics::Image2D&& other) noexcept
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
    size_bytes = other.size_bytes;

    other.width = 0;
    other.height = 0;
    other.is_created = false;
    other.channels = 0;
    other.depth = 0;
    other.layers = 0;
    other.size_bytes = 0;
}

const void* Fleur::Graphics::Image2D::Data() const
{
    return bitmap.Data();
}

void Fleur::Graphics::Image2D::PostCreate(ImagePostCreation& settings)
{
    FL_CORE_ASSERT(settings.data, "[Image2D] data is nullptr");

    ImageBase::PostCreate(settings);

    bitmap = Bitmap<BitmapFormat_UnsignedByte>(settings.width, settings.height, settings.channels);
    memcpy_s(bitmap.Data(), bitmap.GetSizeBytes(), settings.data, settings.width * settings.height * settings.channels * settings.depth);

    is_created = true;
}

Fleur::Graphics::Image2D Fleur::Graphics::Image2D::FromEquirectangularToCross() const
{
    uint32_t face_size = width / 4;
    constexpr float pi = glm::pi<float>();

    Bitmap<BitmapFormat_UnsignedByte> out_bitmap(face_size * 4, face_size * 3, channels);
    struct FacePos
    {
        int x, y;
    };
    const FacePos facePos[12] = {
        {},          // 0
        {1, 0},      // 1 POSITIVE_Y
        {},     {},  // 2,3
        {0, 1},      // 4 NEGATIVE_X
        {3, 1},      // 5 NEGATIVE_Z
        {2, 1},      // 6 POSITIVE_X
        {1, 1},      // 7 POSITIVE_Z
        {},          // 8
        {1, 2},      // 9 NEGATIVE_Y
        {},     {}   // 10,11
    };

    std::vector<float> normU(face_size);
    std::vector<float> normV(face_size);
    for (uint32_t i = 0; i < face_size; ++i)
    {
        normU[i] = ((i + 0.5f) / face_size) * 2.f - 1.f;
        normV[i] = ((i + 0.5f) / face_size) * 2.f - 1.f;
    }

    for (size_t face = 0; face < 12; face++)
    {
        if (face == 0 || face == 2 || face == 3 || face == 8 || face == 10 || face == 11)
        {
            continue;
        }

        const FacePos& fp = facePos[face];

        for (size_t coord_u = 0; coord_u < face_size; coord_u++)
        {
            for (size_t coord_v = 0; coord_v < face_size; coord_v++)
            {
                float u = normU[coord_u];
                float v = normV[coord_v];

                float dx, dy, dz;
                switch (face)
                {
                case 6:  // +X (right)
                    dx = 1.f;
                    dy = -v;
                    dz = u;
                    break;
                case 4:  // -X (left)
                    dx = -1.f;
                    dy = -v;
                    dz = -u;
                    break;
                case 1:  // +Y (top)
                    dx = u;
                    dy = 1.f;
                    dz = -v;
                    break;
                case 9:  // -Y (bottom)
                    dx = u;
                    dy = -1.f;
                    dz = v;
                    break;
                case 7:  // +Z (back)
                    dx = u;
                    dy = -v;
                    dz = -1.f;
                    break;
                case 5:  // -Z (front)
                    dx = -u;
                    dy = -v;
                    dz = 1.f;
                    break;
                }

                float sx = -dz;
                float sy = dx;
                float sz = dy;

                float radius = glm::sqrt(sx * sx + sy * sy + sz * sz);
                float invRadius = 1.0f / radius;

                float phi = atan2(sy, sx);           // [-pi, +pi]
                float theta = acos(sz * invRadius);  // [0, pi]

                float uu = (phi + pi) / (2.f * pi);  // [0,1]
                float vv = theta / pi;               // [0,1]

                float x = static_cast<float>(uu * (width - 1));
                float y = static_cast<float>(vv * (height - 1));


                // bilinear interpolation:
                uint32_t left_x = std::floor(x);

                FL_CORE_ASSERT(left_x >= 0, "");

                uint32_t right_x = std::min(static_cast<uint32_t>(std::floor(left_x + 1)), width - 1);

                uint32_t top_y = std::floor(y);
                uint32_t bottom_y = std::min(static_cast<uint32_t>(top_y + 1), height - 1);

                float shift_x = x - left_x;
                float shift_y = y - top_y;

                // w00 -- w01
                // ----uv----
                // w10 -- w11
                float w00 = (1.f - shift_x) * (1.f - shift_y);
                float w01 = shift_x * (1.f - shift_y);
                float w10 = (1.f - shift_x) * shift_y;
                float w11 = shift_x * shift_y;


                glm::vec4 c00 = bitmap.GetPixel(left_x, top_y);
                glm::vec4 c01 = bitmap.GetPixel(right_x, top_y);
                glm::vec4 c10 = bitmap.GetPixel(left_x, bottom_y);
                glm::vec4 c11 = bitmap.GetPixel(right_x, bottom_y);

                glm::vec4 color = glm::vec4((c00 * w00 + c01 * w01 + c10 * w10 + c11 * w11));

                int out_x = fp.x * face_size + coord_u;
                int out_y = fp.y * face_size + coord_v;
                out_bitmap.SetPixel(out_x, out_y, color);
            }
        }
    }
    return Image2D(name + "_cross_layout", extension, reinterpret_cast<unsigned char*>(out_bitmap.Data()), face_size * 4, face_size * 3, channels, depth);
}

Fleur::Graphics::CubemapImage Fleur::Graphics::Image2D::FromCrossToCubemap() const
{
    uint32_t face_size = width / 4;

    std::array<Image2D, 6> out_faces;
    for (int i = 0; i < 6; i++)
    {
        out_faces[i] = Image2D(name + "_face_" + std::to_string(i), extension, face_size, face_size, channels, depth);
    }

    auto get_stride = [this](TextureFormat format)
    {
        switch (format)
        {
        case TextureFormat::RGB8:
            return 3u;
        case TextureFormat::RGBA8:
            return 4u;
        }
        return 3u;
    };
    uint32_t stride = get_stride(Texture::GetTextureFormat(channels, depth));

    auto upload_face = [&](uint32_t start_x, uint32_t start_y, uint32_t out_face)
    {
        Fleur::Bitmap<BitmapFormat_UnsignedByte> tmp(face_size, face_size, channels);
        for (uint32_t y = 0; y < face_size; ++y)
        {
            for (uint32_t x = 0; x < face_size; ++x)
            {
                glm::vec4 color = bitmap.GetPixel(start_x + x, start_y + y);
                tmp.SetPixel(x, y, color);
            }
        }
        out_faces[out_face].bitmap = std::move(tmp);
    };

    // Face indices: 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z
    upload_face(face_size * 2, face_size, 0);  // +X (right)
    upload_face(0, face_size, 1);              // -X (left)
    upload_face(face_size, 0, 2);              // +Y (top)
    upload_face(face_size, face_size * 2, 3);  // -Y (bottom)
    upload_face(face_size, face_size, 4);      // +Z (front)
    upload_face(face_size * 3, face_size, 5);  // -Z (back)

    return Fleur::Graphics::CubemapImage(name + "_cubemap", extension, std::move(out_faces));
}


// CubemapImage:
Fleur::Graphics::CubemapImage::CubemapImage(std::string_view name, std::string_view ext, std::array<Image2D, 6>&& in_faces)
    : ImageBase(name, ext, in_faces[0].Width(), in_faces[0].Width(), in_faces[0].Channels(), in_faces[0].Depth(), 6)

{
    faces = std::move(in_faces);
}
Fleur::Graphics::CubemapImage::CubemapImage(std::string_view name, std::string_view ext)
    : ImageBase(name, ext, 6)
{
}

Fleur::Graphics::CubemapImage& Fleur::Graphics::CubemapImage::operator=(CubemapImage&& other) noexcept
{
    if (this != &other)
    {
        ImageBase::operator=(std::move(other));
        faces = std::move(other.faces);
    }
    return *this;
}
Fleur::Graphics::CubemapImage::CubemapImage(CubemapImage&& other) noexcept
    : ImageBase(std::move(other))
    , faces(std::move(other.faces))
{
}

const Fleur::Graphics::Image2D& Fleur::Graphics::CubemapImage::GetFace(Face face) const
{
    switch (face)
    {
    case Fleur::Graphics::CubemapImage::Face::Right:
        return faces[0];
    case Fleur::Graphics::CubemapImage::Face::Left:
        return faces[1];
    case Fleur::Graphics::CubemapImage::Face::Top:
        return faces[2];
    case Fleur::Graphics::CubemapImage::Face::Bottom:
        return faces[3];
    case Fleur::Graphics::CubemapImage::Face::Back:
        return faces[4];
    case Fleur::Graphics::CubemapImage::Face::Front:
        return faces[5];
    }
}

const Fleur::Graphics::Image2D* Fleur::Graphics::CubemapImage::Data() const
{
    return reinterpret_cast<const Image2D*>(faces.data());
}

void Fleur::Graphics::CubemapImage::PostCreate(ImagePostCreation& settings)
{
    FL_CORE_ASSERT(settings.data, "[Image2D] data is nullptr");

    ImageBase::PostCreate(settings);
    // faces = reinterpret_cast<const std::array<Fleur::Graphics::Image2D, 6>*>(settings.data);
    //  uint32_t data_chank_size = settings.width * settings.height * settings.channels * settings.depth*;
    /* for (size_t i = 0; i < layers; i++)
     {
         Bitmap<BitmapFormat_UnsignedByte> bitmap = Bitmap<BitmapFormat_UnsignedByte>(settings.width, settings.height, settings.channels);
         faces[i] = Image2D(name, extension, std::move(bitmap), settings.width, settings.height, settings.channels, settings.depth);
     }

     memcpy_s(bitmap.Data(), bitmap.GetSizeBytes(), settings.data, settings.width * settings.height * settings.channels * settings.depth);*/

    is_created = true;
}
