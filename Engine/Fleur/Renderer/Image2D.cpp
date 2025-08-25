#include "Image2D.h"

#include "Color.h"
#include "Services/ServiceLocator.h"


// ImageBase
Fleur::Graphics::ImageBase::ImageBase(std::string_view name, std::string_view ext, uint32_t layers)
    : m_Name(name)
    , m_Extension(ext)
    , m_Width(0)
    , m_Height(0)
    , m_Channels(0)
    , m_Depth(0)
    , m_Layers(layers)
    , m_SizeBytes(0)
    , m_IsCreated(false)
{
}
Fleur::Graphics::ImageBase::ImageBase(std::string_view name, std::string_view ext, uint32_t width, uint32_t height, uint16_t channels, uint16_t depth,
                                      uint32_t layers)
    : m_Name(name)
    , m_Extension(ext)
    , m_Width(width)
    , m_Height(height)
    , m_Channels(channels)
    , m_Depth(depth)
    , m_Layers(layers)
    , m_SizeBytes(width * height * channels * depth * layers)
    , m_IsCreated(false)
{
}

Fleur::Graphics::ImageBase::ImageBase(ImageBase&& other) noexcept
    : m_Name(std::move(other.m_Name))
    , m_Extension(std::move(other.m_Extension))
    , m_Width(other.m_Width)
    , m_Height(other.m_Height)
    , m_Channels(other.m_Channels)
    , m_Depth(other.m_Depth)
    , m_Layers(other.m_Layers)
    , m_SizeBytes(other.m_SizeBytes)
    , m_IsCreated(other.m_IsCreated)
{
    other.m_Width = 0;
    other.m_Height = 0;
    other.m_Channels = 0;
    other.m_Depth = 0;
    other.m_Layers = 0;
    other.m_SizeBytes = 0;
    other.m_IsCreated = false;
}
Fleur::Graphics::ImageBase& Fleur::Graphics::ImageBase::operator=(ImageBase&& other) noexcept
{
    if (this != &other)
    {
        m_Name = std::move(other.m_Name);
        m_Extension = std::move(other.m_Extension);
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_Channels = other.m_Channels;
        m_Depth = other.m_Depth;
        m_Layers = other.m_Layers;
        m_SizeBytes = other.m_SizeBytes;
        m_IsCreated = other.m_IsCreated;

        other.m_Width = 0;
        other.m_Height = 0;
        other.m_Channels = 0;
        other.m_Depth = 0;
        other.m_Layers = 0;
        other.m_SizeBytes = 0;
        other.m_IsCreated = false;
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
    , m_Bitmap(w, h, channels)
{
    FL_CORE_ASSERT(depth > 0 && channels > 0, "Invalid Image data");
    memcpy(m_Bitmap.Data(), data, m_Bitmap.GetSizeBytes());
    m_IsCreated = true;
}
Fleur::Graphics::Image2D::Image2D(std::string_view name, std::string_view ext, Bitmap<BitmapFormat_UnsignedByte>&& in_bitmap, int w, int h, uint16_t channels,
                                  uint16_t depth)
    : ImageBase(name, ext, w, h, channels, depth, 1)
    , m_Bitmap(std::move(in_bitmap))
{
    m_IsCreated = true;
};

Fleur::Graphics::Image2D& Fleur::Graphics::Image2D::operator=(Fleur::Graphics::Image2D&& other) noexcept
{
    if (this != &other)
    {
        m_Bitmap = std::move(other.m_Bitmap);
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_IsCreated = other.m_IsCreated;
        m_Name = std::move(other.m_Name);
        m_Extension = std::move(other.m_Extension);
        m_Channels = other.m_Channels;
        m_Depth = other.m_Depth;
        m_Layers = other.m_Layers;
        m_SizeBytes = other.m_SizeBytes;

        other.m_Width = 0;
        other.m_Height = 0;
        other.m_IsCreated = false;
        other.m_Channels = 0;
        other.m_Depth = 0;
        other.m_Layers = 0;
        other.m_SizeBytes = 0;
    }
    return *this;
}
Fleur::Graphics::Image2D::Image2D(Fleur::Graphics::Image2D&& other) noexcept
{
    m_Bitmap = std::move(other.m_Bitmap);
    m_Width = other.m_Width;
    m_Height = other.m_Height;
    m_IsCreated = other.m_IsCreated;
    m_Name = std::move(other.m_Name);
    m_Extension = std::move(other.m_Extension);
    m_Channels = other.m_Channels;
    m_Depth = other.m_Depth;
    m_Layers = other.m_Layers;
    m_SizeBytes = other.m_SizeBytes;

    other.m_Width = 0;
    other.m_Height = 0;
    other.m_IsCreated = false;
    other.m_Channels = 0;
    other.m_Depth = 0;
    other.m_Layers = 0;
    other.m_SizeBytes = 0;
}

const void* Fleur::Graphics::Image2D::Data() const
{
    return m_Bitmap.Data();
}

void Fleur::Graphics::Image2D::PostCreate(ImagePostCreation& settings)
{
    FL_CORE_ASSERT(settings.data, "[Image2D] data is nullptr");

    ImageBase::PostCreate(settings);

    m_Bitmap = Bitmap<BitmapFormat_UnsignedByte>(settings.width, settings.height, settings.channels);
    memcpy_s(m_Bitmap.Data(), m_Bitmap.GetSizeBytes(), settings.data, settings.width * settings.height * settings.channels * settings.depth);

    m_IsCreated = true;
}

Fleur::Graphics::Image2D Fleur::Graphics::Image2D::FromEquirectangularToCross() const
{
    uint32_t face_size = m_Width / 4;
    constexpr float pi = glm::pi<float>();

    Bitmap<BitmapFormat_UnsignedByte> out_bitmap(face_size * 4, face_size * 3, m_Channels);
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

                float x = static_cast<float>(uu * (m_Width - 1));
                float y = static_cast<float>(vv * (m_Height - 1));


                // bilinear interpolation:
                uint32_t left_x = std::floor(x);

                FL_CORE_ASSERT(left_x >= 0, "");

                uint32_t right_x = std::min(static_cast<uint32_t>(std::floor(left_x + 1)), m_Width - 1);

                uint32_t top_y = std::floor(y);
                uint32_t bottom_y = std::min(static_cast<uint32_t>(top_y + 1), m_Height - 1);

                float shift_x = x - left_x;
                float shift_y = y - top_y;

                // w00 -- w01
                // ----uv----
                // w10 -- w11
                float w00 = (1.f - shift_x) * (1.f - shift_y);
                float w01 = shift_x * (1.f - shift_y);
                float w10 = (1.f - shift_x) * shift_y;
                float w11 = shift_x * shift_y;


                glm::vec4 c00 = m_Bitmap.GetPixel(left_x, top_y);
                glm::vec4 c01 = m_Bitmap.GetPixel(right_x, top_y);
                glm::vec4 c10 = m_Bitmap.GetPixel(left_x, bottom_y);
                glm::vec4 c11 = m_Bitmap.GetPixel(right_x, bottom_y);

                glm::vec4 color = glm::vec4((c00 * w00 + c01 * w01 + c10 * w10 + c11 * w11));

                int out_x = fp.x * face_size + coord_u;
                int out_y = fp.y * face_size + coord_v;
                out_bitmap.SetPixel(out_x, out_y, color);
            }
        }
    }
    return Image2D(m_Name + "_cross_layout", m_Extension, reinterpret_cast<unsigned char*>(out_bitmap.Data()), face_size * 4, face_size * 3, m_Channels, m_Depth);
}

Fleur::Graphics::CubemapImage Fleur::Graphics::Image2D::FromCrossToCubemap() const
{
    uint32_t face_size = m_Width / 4;

    std::array<Image2D, 6> out_faces;
    for (int i = 0; i < 6; i++)
    {
        out_faces[i] = Image2D(m_Name + "_face_" + std::to_string(i), m_Extension, face_size, face_size, m_Channels, m_Depth);
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
    uint32_t stride = get_stride(Texture::GetTextureFormat(m_Channels, m_Depth));

    auto upload_face = [&](uint32_t start_x, uint32_t start_y, uint32_t out_face)
    {
        Fleur::Bitmap<BitmapFormat_UnsignedByte> tmp(face_size, face_size, m_Channels);
        for (uint32_t y = 0; y < face_size; ++y)
        {
            for (uint32_t x = 0; x < face_size; ++x)
            {
                glm::vec4 color = m_Bitmap.GetPixel(start_x + x, start_y + y);
                tmp.SetPixel(x, y, color);
            }
        }
        out_faces[out_face].m_Bitmap = std::move(tmp);
    };

    // Face indices: 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z
    upload_face(face_size * 2, face_size, 0);  // +X (right)
    upload_face(0, face_size, 1);              // -X (left)
    upload_face(face_size, 0, 2);              // +Y (top)
    upload_face(face_size, face_size * 2, 3);  // -Y (bottom)
    upload_face(face_size, face_size, 4);      // +Z (front)
    upload_face(face_size * 3, face_size, 5);  // -Z (back)

    return Fleur::Graphics::CubemapImage(m_Name + "_cubemap", m_Extension, std::move(out_faces));
}


// CubemapImage:
Fleur::Graphics::CubemapImage::CubemapImage(std::string_view name, std::string_view ext, std::array<Image2D, 6>&& in_faces)
    : ImageBase(name, ext, in_faces[0].Width(), in_faces[0].Width(), in_faces[0].Channels(), in_faces[0].Depth(), 6)

{
    m_Faces = std::move(in_faces);
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
        m_Faces = std::move(other.m_Faces);
    }
    return *this;
}
Fleur::Graphics::CubemapImage::CubemapImage(CubemapImage&& other) noexcept
    : ImageBase(std::move(other))
    , m_Faces(std::move(other.m_Faces))
{
}

const Fleur::Graphics::Image2D& Fleur::Graphics::CubemapImage::GetFace(Face face) const
{
    switch (face)
    {
    case Fleur::Graphics::CubemapImage::Face::Right:
        return m_Faces[0];
    case Fleur::Graphics::CubemapImage::Face::Left:
        return m_Faces[1];
    case Fleur::Graphics::CubemapImage::Face::Top:
        return m_Faces[2];
    case Fleur::Graphics::CubemapImage::Face::Bottom:
        return m_Faces[3];
    case Fleur::Graphics::CubemapImage::Face::Back:
        return m_Faces[4];
    case Fleur::Graphics::CubemapImage::Face::Front:
        return m_Faces[5];
    }
}

const Fleur::Graphics::Image2D* Fleur::Graphics::CubemapImage::Data() const
{
    return reinterpret_cast<const Image2D*>(m_Faces.data());
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

    m_IsCreated = true;
}
