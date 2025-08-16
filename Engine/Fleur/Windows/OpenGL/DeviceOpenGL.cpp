#include "DeviceOpenGL.h"

#include "Application.h"
#include "BufferOpenGL.h"
#include "CommandBufferOpenGL.h"
#include "CommandPoolOpenGL.h"
#include "CommandQueueOpenGL.h"
#include "FramebufferOpenGL.h"
#include "ShaderOpenGL.h"
#include "SwapchainOpenGL.h"
// clang-format off
#include "glad/wgl.h"
// clang-format on
#include "Renderer.h"
#include "TextureOpenGL.h"

void OpenGLDebugCallbackFunc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
        return;

    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    FL_CORE_ERROR("---- OpenGL Debug Message ----");
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:
        FL_CORE_ERROR("Source: API");
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        FL_CORE_ERROR("Source: Window System");
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        FL_CORE_ERROR("Source: Shader Compiler");
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        FL_CORE_ERROR("Source: Third Party");
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        FL_CORE_ERROR("Source: Application");
        break;
    case GL_DEBUG_SOURCE_OTHER:
    default:
        FL_CORE_ERROR("Source: Other");
        break;
    }

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        FL_CORE_ERROR("Type: Error");
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        FL_CORE_ERROR("Type: Deprecated Behaviour");
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        FL_CORE_ERROR("Type: Undefined Behaviour");
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        FL_CORE_ERROR("Type: Portability");
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        FL_CORE_ERROR("Type: Performance");
        break;
    case GL_DEBUG_TYPE_MARKER:
        FL_CORE_ERROR("Type: Marker");
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        FL_CORE_ERROR("Type: Push Group");
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        FL_CORE_ERROR("Type: Pop Group");
        break;
    case GL_DEBUG_TYPE_OTHER:
    default:
        FL_CORE_ERROR("Type: Other");
        break;
    }

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
        FL_CORE_ERROR("Severity: High");
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        FL_CORE_ERROR("Severity: Medium");
        break;
    case GL_DEBUG_SEVERITY_LOW:
        FL_CORE_ERROR("Severity: Low");
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        FL_CORE_ERROR("Severity: Notification");
        break;
    default:
        break;
    }

    FL_CORE_ERROR("Message: {0}, Source: {1}, Type: {2}, ID: {3}, Severity: {4}\n", (const char*)message, source, type, id, severity);
}

namespace Fleur::Graphics
{

DeviceOpenGL::DeviceOpenGL()
    : ctx(nullptr)
    , max_textures_units(0)
{
    HWND hwnd = (HWND)Application::instance().GetWindow().GetNativeHandle();
    HDC hdc = GetDC(hwnd);

    int pixel_format_attribs[] = {WGL_DRAW_TO_WINDOW_ARB,
                                  GL_TRUE,
                                  WGL_SUPPORT_OPENGL_ARB,
                                  GL_TRUE,
                                  WGL_DOUBLE_BUFFER_ARB,
                                  GL_TRUE,
                                  WGL_ACCELERATION_ARB,
                                  WGL_FULL_ACCELERATION_ARB,
                                  WGL_PIXEL_TYPE_ARB,
                                  WGL_TYPE_RGBA_ARB,
                                  WGL_COLOR_BITS_ARB,
                                  32,
                                  WGL_DEPTH_BITS_ARB,
                                  24,
                                  WGL_STENCIL_BITS_ARB,
                                  8,
                                  0};
    int pixel_format;
    UINT num_formats;
    int res = wglChoosePixelFormatARB(hdc, pixel_format_attribs, nullptr, 1, &pixel_format, &num_formats);
    if (!res)
    {
        FL_CORE_ERROR("Failed to choose pixel format");
    }

    if (!num_formats)
        FL_CORE_ERROR("Failed to set the OpenGL 4.6 pixel format.");

    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(hdc, pixel_format, sizeof(pfd), &pfd);
    if (!SetPixelFormat(hdc, pixel_format, &pfd))
        FL_CORE_ERROR("Failed to set the OpenGL 4.6 pixel format.");

    // clang-format off
    int ctx_attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB,
        4,
        WGL_CONTEXT_MINOR_VERSION_ARB,
        6,
        WGL_CONTEXT_PROFILE_MASK_ARB,
        WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0,
    };
    // clang-format on

    ctx = wglCreateContextAttribsARB(hdc, 0, ctx_attribs);
    if (!ctx)
        FL_CORE_ERROR("Failed to create OpenGL 4.6 context.");

    if (!wglMakeCurrent(hdc, ctx))
        FL_CORE_ERROR("Failed to activate OpenGL 4.6 rendering context.");

    if (!gladLoaderLoadGL())
        FL_CORE_ERROR("[OpenGL] can't load OoenGL");

    FL_CORE_INFO("OpenGL info:");
    FL_CORE_INFO("  Version: {0}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    FL_CORE_INFO("  GLSL Version: {0}", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    FL_CORE_INFO("  GPU Vendor: {0}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    FL_CORE_INFO("  Renderer: {0}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    // TODO: if debug then enable OpenGL debug callback:
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OpenGLDebugCallbackFunc, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_textures_units);
    FL_CORE_INFO("  Max texture units: {0}", max_textures_units);
    ServiceLocator::instance().GetService<Fleur::Graphics::Renderer>()->MAX_TEXTURES_COUNT = static_cast<uint32_t>(max_textures_units);
    glEnable(GL_DEPTH_TEST);

    // Initial values for color\depth\stencil bufers
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClearDepth(1.0);
    glClearStencil(0);
}

DeviceOpenGL::~DeviceOpenGL()
{
    Release();
}

std::unique_ptr<Device> Device::CreateDevice()
{
    return std::unique_ptr<Device>(new DeviceOpenGL());
}

std::unique_ptr<Surface> DeviceOpenGL::CreateSurface(const void* window)
{
    return std::make_unique<SurfaceOpenGL>(window);
}

std::shared_ptr<Texture> DeviceOpenGL::CreateTexture(std::string_view name, std::string_view ext, TextureFormat format, unsigned char* buffer, int width,
                                                     int height) const
{
    return std::make_shared<TextureOpenGL>(name, ext, buffer, format, width, height, 1);
}

std::shared_ptr<Texture> DeviceOpenGL::CreateTexture(std::string_view name, std::string_view ext) const
{
    return std::make_shared<TextureOpenGL>(name, ext, 1);
}

std::shared_ptr<Texture> DeviceOpenGL::CreateCubemap(const CubemapImage* equirectangular) const
{
    return std::make_shared<TextureOpenGL>(equirectangular->Name(), equirectangular->Ext(), reinterpret_cast<const unsigned char*>(equirectangular->Data()),
                                           Texture::GetTextureFormat(equirectangular->Channels(), equirectangular->Depth()), equirectangular->Width(),
                                           equirectangular->Height(), 6);
}

std::shared_ptr<Texture> DeviceOpenGL::CreateCubemap(const Image2D* cubemap_image) const
{
    return std::make_shared<TextureOpenGL>(cubemap_image->Name(), cubemap_image->Ext(), reinterpret_cast<const unsigned char*>(cubemap_image->Data()),
                                           Texture::GetTextureFormat(cubemap_image->Channels(), cubemap_image->Depth()), cubemap_image->Width(),
                                           cubemap_image->Height(), 6);
}

std::shared_ptr<Texture> DeviceOpenGL::CreateCubemap(std::string_view name, const CubemapInitData& images) const
{
    return std::make_shared<TextureOpenGL>(name, images.right->Ext(), images, Texture::GetTextureFormat(images.right->Channels(), images.right->Depth()),
                                           images.right->Width(), images.right->Height(), 6);
}

std::unique_ptr<Framebuffer> DeviceOpenGL::CreateFramebuffer(std::string_view name, uint32_t width, uint32_t height, uint32_t flags) const
{
    auto fbo = std::unique_ptr<FramebufferOpenGL>(new FramebufferOpenGL(name, width, height, flags));

    if (flags & static_cast<uint32_t>(FramebufferSettings::COLOR))
    {
        fbo->AddColorAttachment(CreateTexture(name.data() + std::string("color_attachment"), "", TextureFormat::RGBA8, nullptr, width, height));
    }
    if (flags & static_cast<uint32_t>(FramebufferSettings::DEPTH_STENCIL))
    {
        fbo->AddDepthAttachment(
            CreateTexture(name.data() + std::string("depth_stencil_attachment"), "", TextureFormat::DEPTH24STENCIL8, nullptr, width, height));
    }
    else
    {
        if (flags & static_cast<uint32_t>(FramebufferSettings::DEPTH))
        {
            fbo->AddDepthAttachment(CreateTexture(name.data() + std::string("depth_attachment"), "", TextureFormat::DEPTH24, nullptr, width, height));
        }

        if (flags & static_cast<uint32_t>(FramebufferSettings::STENCIL))
        {
            fbo->AddStencilAttachment(CreateTexture(name.data() + std::string("stencil_attachment"), "", TextureFormat::STENCIL8, nullptr, width, height));
        }
    }

    return fbo;
}

void DeviceOpenGL::SetVSync(bool active) const
{
    wglSwapIntervalEXT(active);
}

void DeviceOpenGL::Release()
{
    if (ctx)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(ctx);
        ctx = nullptr;
    }
}

std::unique_ptr<Buffer> DeviceOpenGL::CreateBuffer(Buffer::BufferType type, RenderStage stage, size_t size)
{
    return std::unique_ptr<Buffer>(new BufferOpenGL(type, stage, size));
}

std::unique_ptr<CommandQueue> DeviceOpenGL::CreateCommandQueue()
{
    return std::unique_ptr<CommandQueue>(new CommandQueueOpenGL());
}

std::unique_ptr<CommandPool> DeviceOpenGL::CreateCommandPool(const CommandQueue& queue)
{
    return std::unique_ptr<CommandPoolOpenGL>(new CommandPoolOpenGL(queue));
}

std::unique_ptr<CommandBuffer> DeviceOpenGL::CreateCommandBuffer(DepthStencilDescriptor desc)
{
    return std::unique_ptr<CommandBufferOpenGL>(new CommandBufferOpenGL(desc));
}

std::unique_ptr<Swapchain> DeviceOpenGL::CreateSwapchain(std::unique_ptr<Surface> surface)
{
    return std::unique_ptr<Swapchain>(new SwapchainOpenGL(std::move(surface)));
}

Shader* DeviceOpenGL::CreateShader(std::string_view shaderName, Shader::ShaderType type)
{
    // TODO: rework shaders
    if (shaderName.empty())
    {
        FL_CORE_ASSERT(false, "Shader");
        return nullptr;
    }

    std::string extension{};
    if (type == Shader::ShaderType::Vertex)
        extension = ".vert";
    else if (type == Shader::ShaderType::Pixel)
        extension = ".frag";

    auto res = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>()->OpenFile(std::string(shaderName) + extension);

    if (!res)
    {
        FL_CORE_ASSERT(res, "[Shader]->DeviceOpenGL::CreateShader, cant create name")
        return nullptr;
    }

    return new ShaderOpenGL(shaderName, res.value().c_str(), type);
}
}  // namespace Fleur::Graphics
