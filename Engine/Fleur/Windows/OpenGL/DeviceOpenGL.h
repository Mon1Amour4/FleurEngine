#pragma once

#include "Renderer/Device.h"

namespace Fleur::Graphics
{
class DeviceOpenGL final : public Device
{
public:
    FLEUR_NON_COPYABLE_NON_MOVABLE(DeviceOpenGL)

    virtual ~DeviceOpenGL() override;

    virtual std::unique_ptr<Buffer> CreateBuffer(Buffer::EBufferType type, RenderStage stage, size_t size) override;
    virtual std::unique_ptr<CommandQueue> CreateCommandQueue() override;
    virtual std::unique_ptr<CommandPool> CreateCommandPool(const CommandQueue& queue) override;
    virtual std::unique_ptr<CommandBuffer> CreateCommandBuffer(DepthStencilDescriptor descriptor) override;

    virtual std::unique_ptr<Swapchain> CreateSwapchain(std::unique_ptr<Surface> surface) override;

    virtual Shader* CreateShader(std::string_view shaderName, Shader::ShaderType type) override;
    virtual std::unique_ptr<Surface> CreateSurface(const void* window) override;

    virtual std::shared_ptr<Texture> CreateTexture(std::string_view name, std::string_view ext, TextureFormat format, unsigned char* buffer, int width,
                                                   int height) const override;

    virtual std::shared_ptr<Texture> CreateTexture(std::string_view name, std::string_view ext) const override;

    virtual std::shared_ptr<Texture> CreateCubemap(const CubemapImage* equirectangular) const override;
    virtual std::shared_ptr<Texture> CreateCubemap(const Image2D* cubemap_image) const override;
    virtual std::shared_ptr<Texture> CreateCubemap(std::string_view name, const CubemapInitData& images) const override;

    virtual std::unique_ptr<Framebuffer> CreateFramebuffer(std::string_view name, uint32_t width, uint32_t height, uint32_t flags) const override;

    virtual void SetVSync(bool active) const override;

    virtual void Release() override;

protected:
    friend class Device;
    DeviceOpenGL();

private:
    HGLRC ctx;
    int max_textures_units;
};
}  // namespace Fleur::Graphics
