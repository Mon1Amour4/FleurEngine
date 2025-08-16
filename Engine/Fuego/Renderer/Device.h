#pragma once

#include "Buffer.h"
#include "Shader.h"

namespace Fuego::Graphics
{
enum class TextureFormat;
class CommandQueue;
class CommandPool;
class CommandBuffer;
class Swapchain;
class Surface;
class Shader;
class Color;
class Image2D;
class CubemapImage;
class Texture2D;
class TextureCubemap;
class Framebuffer;
enum RenderStage;
enum class FramebufferSettings : uint32_t;
struct CubemapInitData;
struct DepthStencilDescriptor;

class Device
{
public:
    static std::unique_ptr<Device> CreateDevice();
    virtual ~Device() = default;

    virtual std::unique_ptr<Buffer> CreateBuffer(Buffer::BufferType type, RenderStage stage, size_t size) = 0;

    virtual std::unique_ptr<CommandQueue> CreateCommandQueue() = 0;

    virtual std::unique_ptr<CommandPool> CreateCommandPool(const CommandQueue& queue) = 0;

    virtual std::unique_ptr<CommandBuffer> CreateCommandBuffer(DepthStencilDescriptor descriptor) = 0;

    virtual std::unique_ptr<Swapchain> CreateSwapchain(std::unique_ptr<Surface> surface) = 0;

    virtual std::unique_ptr<Surface> CreateSurface(const void* window) = 0;

    virtual std::shared_ptr<Texture> CreateTexture(std::string_view name, std::string_view ext, TextureFormat format, unsigned char* buffer, int width,
                                                   int height) const = 0;

    virtual std::shared_ptr<Texture> CreateTexture(std::string_view name, std::string_view ext) const = 0;

    virtual std::shared_ptr<Texture> CreateCubemap(const CubemapImage* equirectangular) const = 0;
    virtual std::shared_ptr<Texture> CreateCubemap(const Image2D* cubemap_image) const = 0;
    virtual std::shared_ptr<Texture> CreateCubemap(std::string_view name, const CubemapInitData& images) const = 0;


    virtual std::unique_ptr<Framebuffer> CreateFramebuffer(std::string_view name, uint32_t width, uint32_t height, uint32_t flags) const = 0;

    // Yes, I know, raw pointer, so be careful here
    virtual Shader* CreateShader(std::string_view shaderName, Shader::ShaderType type) = 0;

    virtual void SetVSync(bool active) const = 0;

    virtual void Release() = 0;
};
}  // namespace Fuego::Graphics
