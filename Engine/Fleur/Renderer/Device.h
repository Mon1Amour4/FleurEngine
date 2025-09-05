#pragma once

#include "Buffer.h"
#include "Shader.h"

namespace Fleur::Graphics
{
enum class ETextureFormat;
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
enum ERenderStage : uint8_t;
enum class EFramebufferSettings : uint32_t;
struct CubemapInitData;
struct DepthStencilDescriptor;

class Device
{
public:
    [[nodiscard]] static std::unique_ptr<Device> CreateDevice();
    virtual ~Device() = default;

    [[nodiscard]] virtual std::unique_ptr<Buffer> CreateBuffer(Buffer::EBufferType type, ERenderStage stage, size_t size) = 0;

    [[nodiscard]] virtual std::unique_ptr<CommandQueue> CreateCommandQueue() = 0;

    [[nodiscard]] virtual std::unique_ptr<CommandPool> CreateCommandPool(const CommandQueue& queue) = 0;

    [[nodiscard]] virtual std::unique_ptr<CommandBuffer> CreateCommandBuffer(DepthStencilDescriptor descriptor) = 0;

    [[nodiscard]] virtual std::unique_ptr<Swapchain> CreateSwapchain(std::unique_ptr<Surface> surface) = 0;

    [[nodiscard]] virtual std::unique_ptr<Surface> CreateSurface(const void* window) = 0;

    [[nodiscard]] virtual std::shared_ptr<Texture> CreateTexture(std::string_view name, std::string_view ext, ETextureFormat format, unsigned char* buffer,
                                                                 uint32_t width, uint32_t height) const = 0;

    [[nodiscard]] virtual std::shared_ptr<Texture> CreateTexture(std::string_view name, std::string_view ext) const = 0;

    [[nodiscard]] virtual std::shared_ptr<Texture> CreateCubemap(const CubemapImage* equirectangular) const = 0;
    [[nodiscard]] virtual std::shared_ptr<Texture> CreateCubemap(const Image2D* cubemapImage) const = 0;
    [[nodiscard]] virtual std::shared_ptr<Texture> CreateCubemap(std::string_view name, const CubemapInitData& images) const = 0;


    [[nodiscard]] virtual std::unique_ptr<Framebuffer> CreateFramebuffer(std::string_view name, uint32_t width, uint32_t height, uint32_t flags) const = 0;

    // Yes, I know, raw pointer, so be careful here
    [[nodiscard]] virtual Shader* CreateShader(std::string_view shaderName, Shader::EShaderType type) = 0;

    virtual void SetVSync(bool active) const = 0;

    virtual void Release() = 0;
};
}  // namespace Fleur::Graphics
