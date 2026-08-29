#include "Renderer_Vulkan.h"

// This entire .cpp file was so big so it was pain in the ass to navigate throughout
// I've hidden vulkanBackendImpl declaration into .hpp file
#if defined(FLEUR_PLATFORM_WIN)
#define NOMINMAX
#define NOGDI
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <chrono>
#include <utility>

#include "PrivateVulkanImpl.hpp"

namespace
{
constexpr uint32_t kDebugShadowMapTextureSlot = MAX_TEXTURES - 1;
constexpr uint32_t kPointLightShadowFaceCount = 6;
constexpr uint32_t kPointLightShadowMapResolution = 512;
constexpr uint32_t kDirectionalShadowMapResolution = 2048;
constexpr float kPointLightShadowNear = 0.1f;
}  // namespace


// ---------- backend ----------
vk::backend::backend(bool enableValidation, void* pNativeHandle, Fleur::SRect& framebufferSize, Fleur::Graphics::SFLImageView& fallback,
                     uint32_t maxPointLights, uint32_t cascadeCount, Fleur::Graphics::LightSampling directionalLight, Fleur::Graphics::LightSampling pointLight,
                     std::shared_ptr<spdlog::logger> logger)
    : pImpl(std::make_unique<vk::backend::impl>(enableValidation, pNativeHandle, framebufferSize, fallback, maxPointLights, cascadeCount, directionalLight,
                                                pointLight, std::move(logger)))
{
}
vk::backend::~backend() = default;
void vk::backend::SetShaderRegistry(const ShaderRegistry& shaders)
{
    pImpl->setShaderRegistry(shaders);
}
void vk::backend::SetShadowSceneBounds(const Fleur::Graphics::BoundingBox& bounds)
{
    pImpl->setShadowSceneBounds(bounds);
}
void vk::backend::SetShadowSettings(uint32_t cascadeCount, Fleur::Graphics::LightSampling directionalLight, Fleur::Graphics::LightSampling pointLight)
{
    pImpl->setShadowSettings(cascadeCount, directionalLight, pointLight);
}

// TEMP_DEBUG_F4_NORMAL_MAP: remove after normal-map debugging.
void vk::backend::SetNormalMappingEnabled(bool enabled)
{
    pImpl->m_NormalMappingEnabled = enabled;
}
void vk::backend::UploadTextures(Fleur::Graphics::SFLImageViewInfo* pInfo)
{
    pImpl->uploadTextures(pInfo);
}
void vk::backend::CreateSkybox(AssetID id, SFLShaderStages shaderStages)
{
    pImpl->createSkybox(id, shaderStages);
}
void vk::backend::SetSkybox(AssetID id)
{
    pImpl->setSkybox(id);
}
void vk::backend::CreateFloor(AssetID texture, SFLShaderStages shaderStages, float height)
{
    pImpl->createFloor(texture, shaderStages, height);
}
void vk::backend::StartResize()
{
    pImpl->startResize();
}
void vk::backend::EndResize(Fleur::SRect& rect)
{
    pImpl->endResize(rect);
}

void vk::backend::CreatePass(EFLPassKind kind, SFLShaderStages shaderStages)
{
    pImpl->createPass(kind, shaderStages);
}
void vk::backend::CreateShadowPass(EFLShadowPassKind kind, SFLShaderStages shaderStages)
{
    pImpl->createShadowPass(kind, shaderStages);
}

void vk::backend::RegisterModel(const SFLModelRegisterInfo& info)
{
    pImpl->registerModel(info.model, info.vertices, info.vertexCount, info.indices, info.indexCount, info.nodeTransforms, info.nodeTransformCount,
                         info.primitives, info.primitiveCount, info.instances, info.instanceCount);
}
void vk::backend::UnregisterModel(AssetID model)
{
    pImpl->unregisterModel(model);
}
void vk::backend::RemoveTexture(AssetID texture)
{
    // TODO: free the bindless texture slot (slot free-list). Stubbed for now.
}
void vk::backend::BeginFrame(const Fleur::Graphics::RenderFrameData& frameData)
{
    pImpl->beginFrame(frameData);
}
void vk::backend::Draw(AssetID model, const Fleur::Mat4& transform)
{
    pImpl->drawModel(model, transform);
}
void vk::backend::SetFloor(AssetID texture, float height)
{
    pImpl->setFloor(texture, height);
}
void vk::backend::EndFrame()
{
    pImpl->endFrame();
}
void vk::backend::ConfigureDebugDraw(const SFLDebugDrawShaders& shaders)
{
    if (!pImpl->m_DebugDraw->IsInitialized())
    {
        const auto primitivesVertex = pImpl->shaderInfo(shaders.primitives.vertex);
        const auto primitivesFragment = pImpl->shaderInfo(shaders.primitives.fragment);
        const auto geometryVertex = pImpl->shaderInfo(shaders.geometry.vertex);
        const auto geometryFragment = pImpl->shaderInfo(shaders.geometry.fragment);
        vk::ShaderCreateInfo primitivesShaderCreateInfo{.pVertexData = primitivesVertex.shaderCode,
                                                        .vertexSize = primitivesVertex.sizeBytes,
                                                        .pFragmentData = primitivesFragment.shaderCode,
                                                        .fragmentSize = primitivesFragment.sizeBytes};
        vk::ShaderCreateInfo geometryShaderCreateInfo{.pVertexData = geometryVertex.shaderCode,
                                                      .vertexSize = geometryVertex.sizeBytes,
                                                      .pFragmentData = geometryFragment.shaderCode,
                                                      .fragmentSize = geometryFragment.sizeBytes};

        auto& primitivesShader = pImpl->m_ShaderMap.try_emplace("DebugPrimitives").first->second;
        if (!primitivesShader.isInitialized())
            primitivesShader.Init(pImpl->m_Device->GetLogicalDevice(), primitivesShaderCreateInfo);

        auto& geometryShader = pImpl->m_ShaderMap.try_emplace("DebugGeometry").first->second;
        if (!geometryShader.isInitialized())
            geometryShader.Init(pImpl->m_Device->GetLogicalDevice(), geometryShaderCreateInfo);
        pImpl->m_DebugDraw->Create(pImpl->m_Device.get(), pImpl->m_Swapchain.get(), &primitivesShader, &geometryShader, pImpl->m_TextureDescriptorSet,
                                   pImpl->m_MultisampledRenderTarget->GetSamplesCount(), FVkDepthTarget::FindDepthFormat(pImpl->m_Device->GetPhysicalDevice()),
                                   pImpl->m_FramesInFlight);
    }
}
void vk::backend::ConfigureOverlay(SFLShaderStages shaderStages)
{
    const auto vertex = pImpl->shaderInfo(shaderStages.vertex);
    const auto fragment = pImpl->shaderInfo(shaderStages.fragment);
    vk::ShaderCreateInfo overlayShaderCreateInfo{
        .pVertexData = vertex.shaderCode, .vertexSize = vertex.sizeBytes, .pFragmentData = fragment.shaderCode, .fragmentSize = fragment.sizeBytes};

    auto& overlayShader = pImpl->m_ShaderMap.try_emplace("Overlay").first->second;
    if (!overlayShader.isInitialized())
        overlayShader.Init(pImpl->m_Device->GetLogicalDevice(), overlayShaderCreateInfo);

    if (!pImpl->m_OverlayPass->IsInitialized())
        return;

    pImpl->m_OverlayPass->SetShader(&overlayShader);

    const VkDescriptorSetLayout shadowMapLayout = pImpl->m_OverlayPass->GetShadowMapSetLayout();
    VkDescriptorSetAllocateInfo shadowMapAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pImpl->m_DescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &shadowMapLayout,
    };
    VK_CHECK(vkAllocateDescriptorSets(pImpl->m_Device->GetLogicalDevice(), &shadowMapAllocateInfo, &pImpl->m_OverlayShadowMapDescriptorSet));
    pImpl->m_OverlayPass->SetShadowMapDescriptorSet(pImpl->m_OverlayShadowMapDescriptorSet);
}
void vk::backend::DrawLine(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 color, bool depthTest)
{
    pImpl->m_DebugDraw->AddLine(a, b, color);
}
void vk::backend::DrawPoint(Fleur::Vec3 p, Fleur::Vec3 color, float size, bool depthTest)
{
    pImpl->m_DebugDraw->AddPoint(p, color, size);
}

void vk::backend::DrawQuad(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 c, Fleur::Vec3 d, Fleur::Vec4 color, bool depthTest)
{
    pImpl->m_DebugDraw->AddQuad(a, b, c, d, color);
}
void vk::backend::DrawQuad(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 c, Fleur::Vec3 d, uint32_t texture, bool depthTest)
{
    pImpl->m_DebugDraw->AddQuad(a, b, c, d, texture);
}
void vk::backend::DrawBillboard(Fleur::Vec3 center, Fleur::Vec2 size, uint32_t texture, bool depthTest)
{
    pImpl->m_DebugDraw->AddBillboard(center, size, texture);
}
void vk::backend::DrawOverlayQuad(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec2 d, Fleur::Vec4 color)
{
    pImpl->m_OverlayPass->AddQuad(a, b, c, d, color);
}
void vk::backend::DrawOverlayQuad(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec2 d, uint32_t texture)
{
    pImpl->m_OverlayPass->AddQuad(a, b, c, d, texture);
}
void vk::backend::DrawOverlayTriangle(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec4 color)
{
    pImpl->m_OverlayPass->AddTriangle(a, b, c, color);
}
void vk::backend::DrawOverlayTriangle(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, uint32_t texture)
{
    pImpl->m_OverlayPass->AddTriangle(a, b, c, texture);
}
void vk::backend::DrawShadowMapOverlay(Fleur::Vec2 min, Fleur::Vec2 max, int32_t layer)
{
    auto& frame = pImpl->GetCurrentFrame();
    vk::abstraction::DescriptorWriter shadowMapWriter{};
    shadowMapWriter.write_image(0, frame.m_DirectionalLightShadowMap.GetImageView(), pImpl->m_ShadowMapSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    shadowMapWriter.update_set(pImpl->m_Device->GetLogicalDevice(), pImpl->m_OverlayShadowMapDescriptorSet);
    pImpl->m_OverlayPass->SetShadowMapLayer(layer);
    pImpl->m_OverlayPass->AddShadowMapQuad(min, max);
}
void vk::backend::UpdatePointLight(const SFLPointLight* light, uint32_t lightCount)
{
    pImpl->updatePointLight(light, lightCount);
}


// ---------- impl ----------
// clang-format off
vk::backend::impl::impl(bool enableValidation,
                         void* pNativeHandle, Fleur::SRect& framebufferSize,
                         Fleur::Graphics::SFLImageView& fallback, uint32_t maxPointLights, uint32_t cascadeCount,
                         Fleur::Graphics::LightSampling directionalLight, Fleur::Graphics::LightSampling pointLight,
                         std::shared_ptr<spdlog::logger> logger)
    // clang-format on
    : m_Logger(std::move(logger))
    , m_PointLightShadowMaps(maxPointLights)
{
    assert(maxPointLights > 0);
    m_CascadeCount = std::clamp(cascadeCount, 1u, static_cast<uint32_t>(kMaxCascadeCount));
    m_DirectionalLightSampling = directionalLight;
    m_PointLightSampling = pointLight;
    m_MaxPointLights = std::min(maxPointLights, POINT_LIGHTS_CAPACITY);
    std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};
    std::vector<const char*> instanceExtensions{"VK_EXT_debug_utils", "VK_KHR_surface"};
#if defined(FLEUR_PLATFORM_WIN)
    instanceExtensions.push_back("VK_KHR_win32_surface");
#endif
    m_VulkanInstance = createInstance(enableValidation, instanceExtensions, validationLayers);
    setupDebugMessenger();
    myVkCmdBeginDebugUtilsLabelEXT =
        reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(m_VulkanInstance, "vkCmdBeginDebugUtilsLabelEXT"));

    myVkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(m_VulkanInstance, "vkCmdEndDebugUtilsLabelEXT"));

    m_Swapchain = std::make_unique<FVkSwapchain>();
    m_Surface = createSurface(m_VulkanInstance, pNativeHandle);

    SDeviceInfo deviceInfo{};
    deviceInfo.presentationSupport = true;
    deviceInfo.neededQueueFamilyFlags = VK_QUEUE_GRAPHICS_BIT;
    deviceInfo.surface = m_Surface;
    deviceInfo.requiredDeviceExtensions = deviceExtensions;

    m_Device = FVkDevice::CreateSuitableDevice(m_VulkanInstance, deviceInfo);
    m_Device->CreateLogicalDevice(deviceExtensions);

    SetDebugUtilsObjectNameEXT =
        reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(m_Device->GetLogicalDevice(), "vkSetDebugUtilsObjectNameEXT"));

    m_Swapchain->CreateSwapchain(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Surface,
                                 {framebufferSize.x, framebufferSize.y, framebufferSize.width, framebufferSize.height}, m_Device->GetPresentQueueFamilyIndex());
    m_SwapchainImageLayouts.assign(m_Swapchain->GetSwapchainImageCount(), VK_IMAGE_LAYOUT_UNDEFINED);

    m_VertexBuffer = std::make_unique<FVkBuffer>();
    m_IndexBuffer = std::make_unique<FVkBuffer>();
    m_PointLightsBuffer = std::make_unique<FVkBuffer>();

    m_MultisampledRenderTarget = std::make_unique<FVkMultisampler>();
    m_MultisampledRenderTarget->Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Device->GetMemoryTracker(), VK_SAMPLE_COUNT_1_BIT,
                                     m_Swapchain->GetSwapchainExtent().width, m_Swapchain->GetSwapchainExtent().height, m_Swapchain->GetImageFormat());

    m_FramesInFlight = kFramesInFlight;

    VkExtent2D swapchainExtent = m_Swapchain->GetSwapchainExtent();

    m_ImmediateCommandPool = std::make_unique<FVkCommandPool>();
    m_ImmediateCommandPool->Init(m_Device->GetLogicalDevice(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, m_Device->GetGraphicsQueueFamilyIndex());

    createFallbackTexture(fallback);

    m_ImageSampler = createTextureSampler();
    m_Frames.resize(m_FramesInFlight);
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (size_t i = 0; i < m_FramesInFlight; i++)
    {
        Frame& frame = m_Frames[i];

        frame.scene.m_SceneNodeTransformsStorageBuffer.Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Device->GetMemoryTracker(),
                                                            FVkAllocationCategory::Buffer,
                                                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                            NODE_TRANSFORMS_MAX_CUP * sizeof(Fleur::Mat4) + sizeof(Fleur::Mat4), sizeof(Fleur::Mat4));

        frame.scene.m_DirectionalShadowMatricesBuffer.Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Device->GetMemoryTracker(),
                                                           FVkAllocationCategory::Buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                           sizeof(DirectionalShadowMatrices), sizeof(Fleur::Mat4));

        frame.scene.m_SceneDataBuffer.Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Device->GetMemoryTracker(),
                                           FVkAllocationCategory::Buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                           sizeof(SFLSceneDataUBO), sizeof(Fleur::Mat4));

        std::vector<vk::abstraction::DescriptorAllocator::PoolSizeRatio> frame_sizes = {
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_FramesInFlight},          // ssbo
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_FramesInFlight * 2},      // camera data + directional shadow matrices
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_FramesInFlight},  // shadowMap
        };
        frame.frameDescriptors.init(m_Device->GetLogicalDevice(), 1000, frame_sizes);

        frame.m_CommandPools.Init(m_Device->GetLogicalDevice(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_Device->GetGraphicsQueueFamilyIndex());
        frame.m_CommandBuffers.Init(m_Device->GetLogicalDevice(), frame.m_CommandPools.GetCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(m_Device->GetLogicalDevice(), &semaphoreInfo, nullptr, &frame.m_ImagesAvailable) != VK_SUCCESS ||
            vkCreateFence(m_Device->GetLogicalDevice(), &fenceInfo, nullptr, &frame.m_InFlightFences) != VK_SUCCESS)
        {
            DBG_PRINTM("Failed to create semaphores!")
            assert(false);
        }
    }

    createRenderFinishedSemaphores();

    for (Frame& frame : m_Frames)
    {
        frame.m_DirectionalLightShadowMap.Create(m_Device.get(), m_ImmediateCommandPool.get(),
                                                 VkExtent2D{kDirectionalShadowMapResolution, kDirectionalShadowMapResolution}, VK_SAMPLE_COUNT_1_BIT, true,
                                                 m_CascadeCount);
        frame.m_DirectionalLightShadowMapLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    m_ShadowMapSampler = createShadowMapSampler();


    m_DepthRenderTarget.Create(m_Device.get(), m_ImmediateCommandPool.get(), swapchainExtent, m_MultisampledRenderTarget->GetSamplesCount());

    m_VertexBuffer->Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Device->GetMemoryTracker(), FVkAllocationCategory::Buffer,
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 1024u * 1024ul * 512ul, sizeof(Fleur::Graphics::SVertexData));
    FL_CORE_INFO("[Vulkan][VertexLayout] model vertex buffer stride={}", sizeof(Fleur::Graphics::SVertexData));

    m_IndexBuffer->Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Device->GetMemoryTracker(), FVkAllocationCategory::Buffer,
                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 1024u * 1024ul * 256ul, sizeof(uint32_t));

    m_PointLightsBuffer->Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Device->GetMemoryTracker(), FVkAllocationCategory::Buffer,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, m_MaxPointLights * sizeof(SFLPointLight),
                              sizeof(SFLPointLight));

    m_DescriptorSetImageViewsToUpload.resize(m_FramesInFlight);

    {
        FVkSingleTimeCommandBuffer frameCmd = FVkSingleTimeCommandBuffer(m_Device->GetLogicalDevice(), m_ImmediateCommandPool->GetCommandPool());

        frameCmd.TransitionImageLayout(m_MultisampledRenderTarget->GetTexture()->GetImage(), m_Swapchain->GetImageFormat(), VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, 1);

        frameCmd.Submit(m_Device->GetGraphicsQueue());
    }

    m_OverlayPass = std::make_unique<FVkOverlayPass>();
    m_OverlayPass->Create(m_Device.get(), m_Swapchain.get(), m_TextureDescriptorSet, kDebugShadowMapTextureSlot, m_MultisampledRenderTarget->GetSamplesCount(),
                          FVkDepthTarget::FindDepthFormat(m_Device->GetPhysicalDevice()), m_FramesInFlight);

    m_DebugDraw = std::make_unique<FVkDebugDraw>();

    // Create the random offset texture and the descriptor set that owns it.
    m_ShadowMapOffsetTexture.Create(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Device->GetMemoryTracker(),
                                    m_ImmediateCommandPool->GetCommandPool(), m_Device->GetGraphicsQueue(), 3);

    m_PointLightShadowMaps.Create(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Device->GetMemoryTracker(), kPointLightShadowMapResolution,
                                  FindDepthFormat(m_Device->GetPhysicalDevice()));

    m_GBuffer = std::make_unique<GBuffer>(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Device->GetMemoryTracker(), swapchainExtent.width,
                                          swapchainExtent.height, FVkDepthTarget::FindDepthFormat(m_Device->GetPhysicalDevice()));
}

vk::backend::impl::~impl()
{
    vkDeviceWaitIdle(m_Device->GetLogicalDevice());

    m_ShadowMapOffsetTexture.Destroy();
    m_PointLightShadowMaps.Destroy();

    uint32_t framebuffersCount = m_FramesInFlight;

    // 1. Synchronization objects
    for (size_t i = 0; i < framebuffersCount; i++)
    {
        Frame& frame = m_Frames[i];
        vkDestroySemaphore(m_Device->GetLogicalDevice(), frame.m_ImagesAvailable, nullptr);
        vkDestroyFence(m_Device->GetLogicalDevice(), frame.m_InFlightFences, nullptr);
        frame.frameDescriptors.destroy_pools(m_Device->GetLogicalDevice());
    }
    destroyRenderFinishedSemaphores();

    m_GBufferMaterialDescriptors.destroy_pools(m_Device->GetLogicalDevice());

    // 2. CommandBuffer & CommandPool
    // 3. DescriptorSet & DescriptorPool & Descriptor set layout
    vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), m_DescriptorPool, nullptr);

    // 4. Pipeline
    // Pipelines are borrowed from backend-owned FVkPipelineCache instances.

    // Descriptor-set layouts must outlive pipeline layouts, but be destroyed before the device.

    // 5. Swapchain & Framebuffers & swapchain image views

    // 7. All ImageViews
    m_TextureMap.clear();
    m_FallbackCubemapTexture.reset();
    m_MultisampledRenderTarget.reset();
    m_DepthRenderTarget.Destroy();
    m_Swapchain->ReleaseSwapchainImageViews();

    // 8. Buffers
    // 9. Samplers
    vkDestroySampler(m_Device->GetLogicalDevice(), m_ImageSampler, nullptr);
    vkDestroySampler(m_Device->GetLogicalDevice(), m_ShadowMapSampler, nullptr);

    // 10. Swapchain
    m_Swapchain.reset();

    // Frame-owned Vulkan resources must be released before the device.
    m_Frames.clear();
    m_ImmediateCommandPool.reset();
    m_VertexBuffer.reset();
    m_IndexBuffer.reset();
    m_PointLightsBuffer.reset();
    m_Skybox.reset();
    m_Floor.reset();
    m_DebugDraw.reset();
    m_OverlayPass.reset();

    // 11. Surface
    vkDestroySurfaceKHR(m_VulkanInstance, m_Surface, nullptr);

    // 12. LogicalDevice
    m_Device.reset();

    // 13. Debug Utills & Validation Layers
    if (m_ValidationsEnabled)
        destroyDebugUtilsMessenger_EXT(m_VulkanInstance, debugMessenger, nullptr);

    // 14. MeshInstance
    vkDestroyInstance(m_VulkanInstance, nullptr);
}


VkInstance vk::backend::impl::createInstance(bool enableValidation, const std::vector<const char*>& instanceExtensions,
                                             const std::vector<const char*>& validationLayers)
{
    m_ValidationsEnabled = enableValidation;

    uint32_t instanceVersion = 0;
    VK_CHECK(vkEnumerateInstanceVersion(&instanceVersion));
    m_InstanceVersion.Major = VK_API_VERSION_MAJOR(instanceVersion);
    m_InstanceVersion.Minor = VK_API_VERSION_MINOR(instanceVersion);
    m_InstanceVersion.Patch = VK_API_VERSION_PATCH(instanceVersion);
    DBG_PRINT(
        "", "\n[Vulkan] Vulkan Version: Major: " << m_InstanceVersion.Major << ", Minor:" << m_InstanceVersion.Minor << ", Patch: " << m_InstanceVersion.Patch);

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Fleur Engine";
    appInfo.applicationVersion = VULKAN_VERSION;
    appInfo.pEngineName = "Fleur Engine";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    appInfo.apiVersion = VK_MAKE_API_VERSION(0, m_InstanceVersion.Major, m_InstanceVersion.Minor, 0);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

    if (m_ValidationsEnabled)
    {
        // query instance extension properties
        uint32_t availableLayerCount;
        vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(availableLayerCount);
        vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());
        DBG_PRINTM("Vulkan available validation layers:");
        for (size_t i = 0; i < availableLayerCount; i++)
        {
            DBG_PRINT("", '\t' << availableLayers[i].layerName << "  spec_v: " << availableLayers[i].specVersion
                               << "impl_v: " << availableLayers[i].implementationVersion << ' ' << availableLayers[i].description);
        }
        {
            std::list<std::string> layers(validationLayers.begin(), validationLayers.end());
            for (const auto& layer : availableLayers)
            {
                auto it = std::find(layers.begin(), layers.end(), layer.layerName);
                if (it != layers.end())
                {
                    layers.erase(it);
                }
            }
            assert(layers.empty());
        }

        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    // Enable extensions
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> props(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, props.data());

    DBG_PRINTM("Vulkan available extensions:");
    for (size_t i = 0; i < extensionCount; i++)
    {
        DBG_PRINT("", '\t' << props[i].extensionName << " v:" << props[i].specVersion);
    }
    {
        std::list<std::string> extensions(instanceExtensions.begin(), instanceExtensions.end());
        for (const auto& ext : props)
        {
            auto it = std::find(extensions.begin(), extensions.end(), ext.extensionName);
            if (it != extensions.end())
            {
                extensions.erase(it);
            }
        }
        assert(extensions.empty());
    }
    createInfo.enabledExtensionCount = instanceExtensions.size();
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance));

    return m_VulkanInstance;
}

void vk::backend::impl::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}
void vk::backend::impl::setupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (createDebugUtilsMessenger_EXT(m_VulkanInstance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to set up debug messenger");
    }
}

// clang-format off
VkResult vk::backend::impl::createDebugUtilsMessenger_EXT(VkInstance instance, 
                                                          const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                          const VkAllocationCallbacks* pAllocator, 
                                                          VkDebugUtilsMessengerEXT* pDebugMessenger)
// clang-format on
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void vk::backend::impl::destroyDebugUtilsMessenger_EXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}


VkSurfaceKHR vk::backend::impl::createSurface(VkInstance instance, void* pNativeHandle)
{
#if defined(FLEUR_PLATFORM_WIN)
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = reinterpret_cast<HWND>(pNativeHandle);
    createInfo.hinstance = GetModuleHandle(nullptr);

    VK_CHECK(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &m_Surface));
#endif

    return m_Surface;
}

Fleur::Graphics::SFLShaderBytecode vk::backend::impl::shaderInfo(std::string_view name) const
{
    assert(m_ShaderRegistry != nullptr);
    const auto it = m_ShaderRegistry->find(std::string(name));
    assert(it != m_ShaderRegistry->end());
    return {it->second.GetShaderCode(), it->second.GetShaderCodeSizeB()};
}


FVkPipeline* vk::backend::impl::createGraphicsPipeline(const char* shaderKey, Fleur::Graphics::SFLShaderBytecode pVertexInfo,
                                                       Fleur::Graphics::SFLShaderBytecode pFragmentInfo, const vk::GetPipelineInfo& pipelineInfo,
                                                       Fleur::Graphics::SFLShaderBytecode pGeometryInfo)
{
    vk::ShaderCreateInfo shaderCreateInfo{.pVertexData = pVertexInfo.shaderCode,
                                          .vertexSize = pVertexInfo.sizeBytes,
                                          .pGeometryData = pGeometryInfo.shaderCode,
                                          .geometrySize = pGeometryInfo.sizeBytes,
                                          .pFragmentData = pFragmentInfo.shaderCode,
                                          .fragmentSize = pFragmentInfo.sizeBytes};

    auto& shader = m_ShaderMap.try_emplace(shaderKey).first->second;
    if (!shader.isInitialized())
        shader.Init(m_Device->GetLogicalDevice(), shaderCreateInfo);
    if (!shader.isInitialized())
    {
        FL_CORE_ERROR("[Vulkan] Shader '{}' failed reflection; pipeline creation skipped", shaderKey);
        return nullptr;
    }

    auto& reflectedLayout = m_PipelineLayouts[shaderKey];
    if (!reflectedLayout)
    {
        reflectedLayout = std::make_shared<FVkPipelineLayout>();
        reflectedLayout->Init(m_Device->GetLogicalDevice(), shader);
    }

    if (std::string_view(shaderKey) == "opaque")
    {
        m_OpaquePipelineLayout = reflectedLayout;
        initializeOpaqueDescriptorSets();
    }
    else if (std::string_view(shaderKey) == "shadow")
    {
        m_ShadowPipelineLayout = reflectedLayout;
    }
    else if (std::string_view(shaderKey) == "gbuffer")
    {
        m_GBufferPipelineLayout = reflectedLayout;
    }
    else if (std::string_view(shaderKey) == "deferredLighting")
    {
        m_DeferredLightingPipelineLayout = reflectedLayout;
    }

    return &m_PipelineCache.Get(shader, pipelineInfo, reflectedLayout);
}

FVkPipeline* vk::backend::impl::createOpaquePipeline(Fleur::Graphics::SFLShaderBytecode pVertexInfo, Fleur::Graphics::SFLShaderBytecode pFragmentInfo,
                                                     VkSampleCountFlagBits samplesCount)
{
    vk::GetPipelineInfo pipelineInfo{};
    pipelineInfo.blendEnable = false;
    pipelineInfo.cullMode = VK_CULL_MODE_NONE;
    pipelineInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    pipelineInfo.depthTestEnable = true;
    pipelineInfo.depthWriteEnable = true;
    pipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineInfo.samplesCount = samplesCount;
    pipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInfo.colorFormat = m_Swapchain->GetImageFormat();
    pipelineInfo.depthFormat = FVkDepthTarget::FindDepthFormat(m_Device->GetPhysicalDevice());

    return createGraphicsPipeline("opaque", pVertexInfo, pFragmentInfo, pipelineInfo);
}

FVkPipeline* vk::backend::impl::createTransparentPipeline(Fleur::Graphics::SFLShaderBytecode pVertexInfo, Fleur::Graphics::SFLShaderBytecode pFragmentInfo,
                                                          VkSampleCountFlagBits samplesCount)
{
    vk::GetPipelineInfo pipelineInfo{};
    pipelineInfo.blendEnable = true;
    pipelineInfo.cullMode = VK_CULL_MODE_NONE;
    pipelineInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    pipelineInfo.depthTestEnable = true;
    pipelineInfo.depthWriteEnable = false;
    pipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineInfo.samplesCount = samplesCount;
    pipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInfo.colorFormat = m_Swapchain->GetImageFormat();
    pipelineInfo.depthFormat = FVkDepthTarget::FindDepthFormat(m_Device->GetPhysicalDevice());

    return createGraphicsPipeline("opaque", pVertexInfo, pFragmentInfo, pipelineInfo);
}

FVkPipeline* vk::backend::impl::createShadowPipeline(Fleur::Graphics::SFLShaderBytecode pVertexInfo, Fleur::Graphics::SFLShaderBytecode pGeometryInfo,
                                                     Fleur::Graphics::SFLShaderBytecode pFragmentInfo, VkSampleCountFlagBits samplesCount)
{
    vk::GetPipelineInfo pipelineInfo{};
    pipelineInfo.blendEnable = false;

    pipelineInfo.cullMode = VK_CULL_MODE_NONE;
    pipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    pipelineInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    pipelineInfo.depthTestEnable = true;
    pipelineInfo.depthWriteEnable = true;

    pipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Shadow map usually should not use swapchain/MSAA sample count.
    pipelineInfo.samplesCount = VK_SAMPLE_COUNT_1_BIT;

    // Depth-only dynamic rendering.
    pipelineInfo.colorAttachmentCount = 0;
    pipelineInfo.colorFormat = VK_FORMAT_UNDEFINED;
    pipelineInfo.depthFormat = FVkDepthTarget::FindDepthFormat(m_Device->GetPhysicalDevice());

    // Shadow bias. Values are starting points, tune later.
    pipelineInfo.depthBiasEnable = true;
    pipelineInfo.depthBiasConstantFactor = 1.25f;
    pipelineInfo.depthBiasClamp = 0.0f;
    pipelineInfo.depthBiasSlopeFactor = 1.75f;

    return createGraphicsPipeline("shadow", pVertexInfo, pFragmentInfo, pipelineInfo, pGeometryInfo);
}

FVkPipeline* vk::backend::impl::createGBufferPipeline(Fleur::Graphics::SFLShaderBytecode pVertexInfo, Fleur::Graphics::SFLShaderBytecode pFragmentInfo)
{
    vk::GetPipelineInfo pipelineInfo{};
    pipelineInfo.blendEnable = false;
    pipelineInfo.depthTestEnable = true;
    pipelineInfo.depthWriteEnable = true;
    pipelineInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    pipelineInfo.cullMode = VK_CULL_MODE_NONE;
    pipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineInfo.samplesCount = VK_SAMPLE_COUNT_1_BIT;
    pipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInfo.colorAttachmentCount = 3;
    pipelineInfo.colorFormats[0] = VK_FORMAT_R16G16B16A16_SFLOAT;
    pipelineInfo.colorFormats[1] = VK_FORMAT_R16G16B16A16_SFLOAT;
    pipelineInfo.colorFormats[2] = VK_FORMAT_R8G8B8A8_UNORM;
    pipelineInfo.depthFormat = FVkDepthTarget::FindDepthFormat(m_Device->GetPhysicalDevice());

    FVkPipeline* pipeline = createGraphicsPipeline("gbuffer", pVertexInfo, pFragmentInfo, pipelineInfo);

    if (!m_GBufferMaterialDescriptorsInitialized)
    {
        std::array<vk::abstraction::DescriptorAllocator::PoolSizeRatio, 1> poolSizes{{
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2},  // diffuse + specular per primitive
        }};
        m_GBufferMaterialDescriptors.init(m_Device->GetLogicalDevice(), 1024, poolSizes);
        m_GBufferMaterialDescriptorsInitialized = true;
    }

    return pipeline;
}

FVkPipeline* vk::backend::impl::createDeferredLightingPipeline(Fleur::Graphics::SFLShaderBytecode pVertexInfo,
                                                                 Fleur::Graphics::SFLShaderBytecode pFragmentInfo)
{
    vk::GetPipelineInfo pipelineInfo{};
    pipelineInfo.blendEnable = false;
    pipelineInfo.depthTestEnable = false;
    pipelineInfo.depthWriteEnable = false;
    pipelineInfo.cullMode = VK_CULL_MODE_NONE;
    pipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineInfo.samplesCount = VK_SAMPLE_COUNT_1_BIT;
    pipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInfo.colorFormat = m_Swapchain->GetImageFormat();
    pipelineInfo.depthFormat = VK_FORMAT_UNDEFINED;
    return createGraphicsPipeline("deferredLighting", pVertexInfo, pFragmentInfo, pipelineInfo);
}

VkShaderModule vk::backend::impl::createShaderModule(Fleur::Graphics::SFLShaderBytecode* pShaderInfo)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = pShaderInfo->sizeBytes;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(pShaderInfo->shaderCode);

    VkShaderModule shaderModule;
    VK_CHECK(vkCreateShaderModule(m_Device->GetLogicalDevice(), &createInfo, nullptr, &shaderModule));

    return shaderModule;
}


void vk::backend::impl::createTextureDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = MAX_TEXTURES + m_MaxPointLights + 6;

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
                                         .maxSets = 6,
                                        .poolSizeCount = poolSizes.size(),
                                        .pPoolSizes = poolSizes.data()};

    VK_CHECK(vkCreateDescriptorPool(m_Device->GetLogicalDevice(), &poolInfo, nullptr, &m_DescriptorPool));
}
void vk::backend::impl::createTextureDescriptorSets()
{
    assert(m_OpaquePipelineLayout != nullptr);

    // ---------- textures ----------
    const std::array<VkDescriptorSetLayout, 4> layouts{
        m_OpaquePipelineLayout->GetSetLayout(1),
        m_OpaquePipelineLayout->GetSetLayout(3),
        m_OpaquePipelineLayout->GetSetLayout(5),
        m_OpaquePipelineLayout->GetSetLayout(6),
    };
    for (VkDescriptorSetLayout layout : layouts) assert(layout != VK_NULL_HANDLE);

    VkDescriptorSetAllocateInfo texturesDescriptorSetAllocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = m_DescriptorPool, .descriptorSetCount = 1, .pSetLayouts = &layouts[0]};

    VK_CHECK(vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &texturesDescriptorSetAllocInfo, &m_TextureDescriptorSet));

    // TODO: keep a dedicated fallback sampled image per descriptor class (color/depth/shadow)
    // instead of reusing a single placeholder texture everywhere.
    VkImageView placeholderImageView = m_TextureMap[m_FallbackTextureIdx].GetImageView();
    VkDescriptorImageInfo imageSamplerInfo{};
    imageSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageSamplerInfo.imageView = placeholderImageView;
    imageSamplerInfo.sampler = m_ImageSampler;

    VkWriteDescriptorSet descriptorImageWrites{};
    descriptorImageWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorImageWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorImageWrites.dstSet = m_TextureDescriptorSet;
    descriptorImageWrites.dstBinding = 0;
    descriptorImageWrites.dstArrayElement = 0;
    descriptorImageWrites.descriptorCount = 1;
    descriptorImageWrites.pImageInfo = &imageSamplerInfo;

    vkUpdateDescriptorSets(m_Device->GetLogicalDevice(), 1, &descriptorImageWrites, 0, nullptr);

    VkDescriptorSetAllocateInfo pointLightsDescriptorSetAllocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = m_DescriptorPool, .descriptorSetCount = 1, .pSetLayouts = &layouts[1]};

    VK_CHECK(vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &pointLightsDescriptorSetAllocInfo, &m_PointLightsDescriptorSet));

    VkDescriptorBufferInfo pointLightBufferInfo{};
    pointLightBufferInfo.buffer = m_PointLightsBuffer->GetBuffer();
    pointLightBufferInfo.offset = 0;
    pointLightBufferInfo.range = m_MaxPointLights * sizeof(Fleur::Graphics::SFLPointLight);

    VkWriteDescriptorSet pointLightDescriptorWrites{};
    pointLightDescriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    pointLightDescriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pointLightDescriptorWrites.dstSet = m_PointLightsDescriptorSet;
    pointLightDescriptorWrites.dstBinding = 0;
    pointLightDescriptorWrites.dstArrayElement = 0;
    pointLightDescriptorWrites.descriptorCount = 1;
    pointLightDescriptorWrites.pBufferInfo = &pointLightBufferInfo;

    vkUpdateDescriptorSets(m_Device->GetLogicalDevice(), 1, &pointLightDescriptorWrites, 0, nullptr);

    VkDescriptorSetAllocateInfo randomOffsetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = m_DescriptorPool, .descriptorSetCount = 1, .pSetLayouts = &layouts[2]};
    VK_CHECK(vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &randomOffsetAllocateInfo, &m_ShadowMapOffsetDescriptorSet));
    m_ShadowMapOffsetTexture.UpdateDescriptorSet(m_ShadowMapOffsetDescriptorSet);

    VkDescriptorSetAllocateInfo pointShadowAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = m_DescriptorPool, .descriptorSetCount = 1, .pSetLayouts = &layouts[3]};
    VK_CHECK(vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &pointShadowAllocateInfo, &m_PointLightShadowMapsDescriptorSet));

    std::vector<VkDescriptorImageInfo> imageInfos(m_MaxPointLights);
    for (uint32_t index = 0; index < m_MaxPointLights; ++index)
    {
        imageInfos[index].sampler = m_ShadowMapSampler;
        imageInfos[index].imageView = m_PointLightShadowMaps.GetCubeImageView(index);
        imageInfos[index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkWriteDescriptorSet pointShadowWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    pointShadowWrite.dstSet = m_PointLightShadowMapsDescriptorSet;
    pointShadowWrite.dstBinding = 0;
    pointShadowWrite.descriptorCount = m_MaxPointLights;
    pointShadowWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pointShadowWrite.pImageInfo = imageInfos.data();
    vkUpdateDescriptorSets(m_Device->GetLogicalDevice(), 1, &pointShadowWrite, 0, nullptr);
}

void vk::backend::impl::uploadTextures(Fleur::Graphics::SFLImageViewInfo* pInfo)
{
    for (size_t i = 0; i < pInfo->count; i++)
    {
        auto& imageView = pInfo->pData[i];
        if (m_TextureMap.contains(imageView.ID))
            continue;

        VkFormat format = m_Device->GetTextureFormat(imageView.channels, imageView.srgb);
        VkImageAspectFlagBits aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        uint32_t layerSize = imageView.w * imageView.h * imageView.channels;
        uint32_t imageSize = layerSize * imageView.layerCount;
        uint32_t mipMapLevel = 1;
        if (imageView.layerCount == 1)
            mipMapLevel = CalculateMipMapLevel(imageView.w, imageView.h);
        auto& gpuTexture = m_TextureMap.emplace(imageView.ID, FVkTexture()).first->second;

        createTexture(imageView, gpuTexture, format, aspect, mipMapLevel, imageView.layerCount);

        updateTextureDescriptorSet(m_TextureDescriptorSet, imageView.ID, gpuTexture.GetImageView(), m_ImageSampler);

        if (imageView.ID == m_FloorTextureIdx && !m_FloorTextureWasLoaded && m_Floor)
        {
            m_Floor->SetFloor(gpuTexture.GetImageView());
            m_FloorTextureWasLoaded = true;
        }
    }
}

void vk::backend::impl::SFLCmdBuffer::invalidate()
{
    for (size_t i = 0; i < validation.size(); i++)
    {
        validation[i] = false;
    }
}
bool vk::backend::impl::SFLCmdBuffer::areValid()
{
    for (size_t i = 0; i < validation.size(); i++)
    {
        if (validation[i] == false)
            return false;
    }
    return true;
}

VkImageView vk::backend::impl::createTextureImageView(VkImage& image, VkFormat format)
{
    return createImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT);
}
VkImageView vk::backend::impl::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    // TODO destroy it upon application termination
    VkImageView imageView;
    VK_CHECK(vkCreateImageView(m_Device->GetLogicalDevice(), &viewInfo, nullptr, &imageView));

    return imageView;
}
VkSampler vk::backend::impl::createTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_Device->GetPhysicalDevice(), &properties);

    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    VkSampler sampler{};
    VK_CHECK(vkCreateSampler(m_Device->GetLogicalDevice(), &samplerInfo, nullptr, &sampler));

    return sampler;
}

VkSampler vk::backend::impl::createShadowMapSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkSampler sampler{};
    VK_CHECK(vkCreateSampler(m_Device->GetLogicalDevice(), &samplerInfo, nullptr, &sampler));

    return sampler;
}

void vk::backend::impl::createRenderFinishedSemaphores()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    m_RenderFinished.assign(m_Swapchain->GetSwapchainImageCount(), VK_NULL_HANDLE);
    for (VkSemaphore& semaphore : m_RenderFinished) VK_CHECK(vkCreateSemaphore(m_Device->GetLogicalDevice(), &semaphoreInfo, nullptr, &semaphore));
}

void vk::backend::impl::destroyRenderFinishedSemaphores()
{
    vkQueueWaitIdle(m_Device->GetPresentQueue());

    for (VkSemaphore semaphore : m_RenderFinished)
    {
        if (semaphore != VK_NULL_HANDLE)
            vkDestroySemaphore(m_Device->GetLogicalDevice(), semaphore, nullptr);
    }

    m_RenderFinished.clear();
}


void vk::backend::impl::createFallbackTexture(Fleur::Graphics::SFLImageView& view)
{
    VkFormat format = m_Device->GetTextureFormat(view.channels);
    VkImageAspectFlagBits aspect = VK_IMAGE_ASPECT_COLOR_BIT;

    m_FallbackTextureIdx = view.ID;
    auto fallbackTexture = &m_TextureMap.emplace(m_FallbackTextureIdx, FVkTexture()).first->second;

    uint32_t mipMapCount = CalculateMipMapLevel(view.w, view.h);
    createTexture(view, *fallbackTexture, format, aspect, mipMapCount, 1);


    // ---------- cubemap texture placeholder ----------
    m_FallbackCubemapTexture = std::make_unique<FVkTexture>();

    VkImageAspectFlagBits cubemapAspect = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t cubemapMimMapCount = 1;
    uint32_t layerSize = view.w * view.h * view.channels;
    uint32_t imageSize = layerSize * CUBEMAP_LAYERS_COUNT;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imageInfo.extent.width = view.w;
    imageInfo.extent.height = view.h;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = cubemapMimMapCount;
    imageInfo.arrayLayers = CUBEMAP_LAYERS_COUNT;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    std::vector<char> buffer(imageSize);
    for (size_t i = 0; i < CUBEMAP_LAYERS_COUNT; i++)
    {
        memcpy(buffer.data() + (layerSize * i), view.pData, layerSize);
    }

    FVkBuffer stagingBuffer{};
    stagingBuffer.Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Device->GetMemoryTracker(), FVkAllocationCategory::Staging,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT, imageSize, layerSize);
    stagingBuffer.MemCopy(buffer.data(), imageSize);

    VkImage cubemapImage = m_FallbackCubemapTexture->CreateImage(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Device->GetMemoryTracker(),
                                                                 FVkAllocationCategory::Texture, imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cubemapAspect);

    {
        FVkSingleTimeCommandBuffer frameCmd = FVkSingleTimeCommandBuffer(m_Device->GetLogicalDevice(), m_ImmediateCommandPool->GetCommandPool());
        frameCmd.TransitionImageLayout(cubemapImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cubemapAspect, cubemapMimMapCount,
                                       CUBEMAP_LAYERS_COUNT);
        frameCmd.CopyBufferToImage(stagingBuffer.GetBuffer(), cubemapImage, VkExtent2D{view.w, view.h}, layerSize, CUBEMAP_LAYERS_COUNT);
        frameCmd.TransitionImageLayout(cubemapImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cubemapAspect,
                                       cubemapMimMapCount, CUBEMAP_LAYERS_COUNT);
        frameCmd.Submit(m_Device->GetGraphicsQueue());
    }
    m_FallbackCubemapTexture->CreateImageView();
}

void vk::backend::impl::updateTextureDescriptorSet(VkDescriptorSet& set, uint32_t idx, VkImageView imageView, VkSampler& sampler)
{
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrites{};
    descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites.dstSet = set;
    descriptorWrites.dstBinding = 0;
    descriptorWrites.dstArrayElement = idx;
    descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites.descriptorCount = 1;
    descriptorWrites.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_Device->GetLogicalDevice(), 1, &descriptorWrites, 0, nullptr);
}


void vk::backend::impl::createTexture(Fleur::Graphics::SFLImageView& view, FVkTexture& texture, VkFormat format, VkImageAspectFlags aspect, uint32_t mipLevels,
                                      uint32_t layerCount)
{
    uint32_t channels = GetChannelsNumFromFormat(format);

    if (mipLevels == 0)
        mipLevels = 1;

    const VkImageUsageFlags attachmentUsage = aspect == VK_IMAGE_ASPECT_DEPTH_BIT ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                                                                                   : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = VkExtent3D{.width = view.w, .height = view.h, .depth = 1},
        .mipLevels = mipLevels,
        .arrayLayers = layerCount,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    if (layerCount == 6)
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imageInfo.usage = (mipLevels > 1) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                                      : VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkImage vkImage = texture.CreateImage(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Device->GetMemoryTracker(),
                                          FVkAllocationCategory::Texture, imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, aspect);

    VkDeviceSize layerSize = view.w * view.h * channels;
    VkDeviceSize imageSize = layerSize * view.layerCount;


    FVkBuffer stagingBuffer{};
    stagingBuffer.Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Device->GetMemoryTracker(), FVkAllocationCategory::Staging,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT, imageSize, layerSize);

    stagingBuffer.MemCopy(view.pData, imageSize);

    {
        FVkSingleTimeCommandBuffer frameCmd = FVkSingleTimeCommandBuffer(m_Device->GetLogicalDevice(), m_ImmediateCommandPool->GetCommandPool());
        frameCmd.TransitionImageLayout(vkImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, aspect, mipLevels, layerCount);
        frameCmd.CopyBufferToImage(stagingBuffer.GetBuffer(), vkImage, VkExtent2D{view.w, view.h}, layerSize, layerCount);
        if (mipLevels > 1)
        {
            frameCmd.GenerateMipMaps(m_Device->GetPhysicalDevice(), vkImage, format, view.w, view.h, mipLevels);
        }
        else
        {
            frameCmd.TransitionImageLayout(vkImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, aspect, mipLevels,
                                           layerCount);
        }

        frameCmd.Submit(m_Device->GetGraphicsQueue());
    }

    texture.CreateImageView();
}

void vk::backend::impl::startResize()
{
    m_WindowResizeIsInProgress = true;
    m_Logger->info("StartResize");
}
void vk::backend::impl::endResize(Fleur::SRect& rect)
{
    m_WindowResizeIsInProgress = false;
    vkDeviceWaitIdle(m_Device->GetLogicalDevice());
    m_Swapchain->OnWindowResized(rect);
    m_DepthRenderTarget.Recreate({rect.width, rect.height}, m_MultisampledRenderTarget->GetSamplesCount());
    for (Frame& frame : m_Frames)
    {
        frame.m_DirectionalLightShadowMap.Recreate({kDirectionalShadowMapResolution, kDirectionalShadowMapResolution}, VK_SAMPLE_COUNT_1_BIT, true,
                                                   m_CascadeCount);
        frame.m_DirectionalLightShadowMapLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    m_SwapchainImageLayouts.assign(m_Swapchain->GetSwapchainImageCount(), VK_IMAGE_LAYOUT_UNDEFINED);
    updateShadowMapDescriptorSets();
    updateTextureDescriptorSet(m_TextureDescriptorSet, kDebugShadowMapTextureSlot, m_Frames.front().m_DirectionalLightShadowMap.GetImageView(),
                               m_ShadowMapSampler);
    m_Logger->info("EndResize");
}

void vk::backend::impl::createSkybox(AssetID id, SFLShaderStages shaderStages)
{
    if (m_Skybox)
        return;

    assert(!shaderStages.vertex.empty());
    assert(!shaderStages.fragment.empty());

    const auto vertex = shaderInfo(shaderStages.vertex);
    const auto fragment = shaderInfo(shaderStages.fragment);
    vk::ShaderCreateInfo shaderCreateInfo{
        .pVertexData = vertex.shaderCode, .vertexSize = vertex.sizeBytes, .pFragmentData = fragment.shaderCode, .fragmentSize = fragment.sizeBytes};

    auto& skyboxShader = m_ShaderMap.try_emplace("skybox").first->second;
    skyboxShader.Init(m_Device->GetLogicalDevice(), shaderCreateInfo);
    if (!skyboxShader.isInitialized())
    {
        FL_CORE_ERROR("[Vulkan] Skybox shader failed reflection; skybox creation skipped");
        return;
    }

    m_Skybox = std::make_unique<FVkSkybox>();
    m_Skybox->Create(m_Device.get(), m_Swapchain.get(), m_FallbackCubemapTexture->GetImageView(), &skyboxShader, m_MultisampledRenderTarget->GetSamplesCount(),
                     FVkDepthTarget::FindDepthFormat(m_Device->GetPhysicalDevice()));
}
void vk::backend::impl::setSkybox(AssetID id)
{
    m_Skybox->SetSkybox(m_TextureMap[id].GetImageView());
}

void vk::backend::impl::createFloor(AssetID texture, SFLShaderStages shaderStages, float height)
{
    if (m_Floor)
        return;

    assert(!shaderStages.vertex.empty() && !shaderStages.fragment.empty());

    const auto vertex = shaderInfo(shaderStages.vertex);
    const auto fragment = shaderInfo(shaderStages.fragment);
    vk::ShaderCreateInfo shaderCreateInfo{
        .pVertexData = vertex.shaderCode, .vertexSize = vertex.sizeBytes, .pFragmentData = fragment.shaderCode, .fragmentSize = fragment.sizeBytes};
    auto& floorShader = m_ShaderMap.try_emplace("floor").first->second;
    floorShader.Init(m_Device->GetLogicalDevice(), shaderCreateInfo);
    if (!floorShader.isInitialized())
    {
        FL_CORE_ERROR("[Vulkan] Floor shader failed reflection; floor creation skipped");
        return;
    }

    m_FloorTextureIdx = texture;
    m_Floor = std::make_unique<FVkFloor>();
    uint32_t idx = m_FloorTextureIdx;
    if (m_TextureMap.find(texture) == m_TextureMap.end())
        idx = m_FallbackTextureIdx;

    m_Floor->Create(m_Device.get(), m_Swapchain.get(), &floorShader, m_TextureMap[idx].GetImageView(), height, m_MultisampledRenderTarget->GetSamplesCount(),
                    FVkDepthTarget::FindDepthFormat(m_Device->GetPhysicalDevice()));
}

void vk::backend::impl::setFloor(AssetID texture, float height)
{
    if (m_Floor)
        m_Floor->SetFloor(m_TextureMap[texture].GetImageView(), height);
}

void vk::backend::impl::createPass(EFLPassKind kind, SFLShaderStages shaderStages)
{
    const auto vertex = shaderInfo(shaderStages.vertex);
    const auto fragment = shaderInfo(shaderStages.fragment);

    if (kind == EFLPassKind::Opaque)
    {
        // TODO if pipeline already exists, need to release it
        m_OpaquePipeline = createOpaquePipeline(vertex, fragment, m_MultisampledRenderTarget->GetSamplesCount());
        m_TransparentPipeline = createTransparentPipeline(vertex, fragment, m_MultisampledRenderTarget->GetSamplesCount());
    }
    else if (kind == EFLPassKind::Deferred)
    {
        m_GBufferPipeline = createGBufferPipeline(vertex, fragment);
    }
    else if (kind == EFLPassKind::DeferredLighting)
    {
        m_DeferredLightingPipeline = createDeferredLightingPipeline(vertex, fragment);

        const VkDescriptorSetLayout layout = m_DeferredLightingPipelineLayout->GetSetLayout(1);
        VkDescriptorSetAllocateInfo allocInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                              .descriptorPool = m_DescriptorPool,
                                              .descriptorSetCount = 1,
                                              .pSetLayouts = &layout};
        VK_CHECK(vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &allocInfo, &m_GBufferLightingDescriptorSet));

        vk::abstraction::DescriptorWriter writer{};
        writer.write_image(0, m_GBuffer->GetPositionTexture().GetImageView(), m_ImageSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.write_image(1, m_GBuffer->GetNormalTexture().GetImageView(), m_ImageSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.write_image(2, m_GBuffer->GetAlbedoTexture().GetImageView(), m_ImageSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.update_set(m_Device->GetLogicalDevice(), m_GBufferLightingDescriptorSet);
    }
}

void vk::backend::impl::createShadowPass(EFLShadowPassKind kind, SFLShaderStages shaderStages)
{
    const auto vertex = shaderInfo(shaderStages.vertex);
    const auto fragment = shaderInfo(shaderStages.fragment);

    if (kind == EFLShadowPassKind::Directional)
    {
        const auto geometry = shaderInfo(shaderStages.geometry);
        m_ShadowPipeline = createShadowPipeline(vertex, geometry, fragment, VK_SAMPLE_COUNT_1_BIT);

        for (Frame& frame : m_Frames)
        {
            frame.scene.m_DirectionalShadowMatricesDescriptor =
                frame.frameDescriptors.allocate(m_Device->GetLogicalDevice(), m_ShadowPipelineLayout->GetSetLayout(1), 1);

            vk::abstraction::DescriptorWriter matricesWriter{};
            matricesWriter.write_buffer(0, frame.scene.m_DirectionalShadowMatricesBuffer.GetBuffer(), sizeof(DirectionalShadowMatrices), 0,
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            matricesWriter.update_set(m_Device->GetLogicalDevice(), frame.scene.m_DirectionalShadowMatricesDescriptor);
        }
    }
    else if (kind == EFLShadowPassKind::PointLight)
    {
        const auto geometry = shaderInfo(shaderStages.geometry);
        vk::ShaderCreateInfo shaderCreateInfo{.pVertexData = vertex.shaderCode,
                                              .vertexSize = vertex.sizeBytes,
                                              .pGeometryData = geometry.shaderCode,
                                              .geometrySize = geometry.sizeBytes,
                                              .pFragmentData = fragment.shaderCode,
                                              .fragmentSize = fragment.sizeBytes};

        auto& pointLightShadowShader = m_ShaderMap.try_emplace("PointLightShadow").first->second;
        if (!pointLightShadowShader.isInitialized())
            pointLightShadowShader.Init(m_Device->GetLogicalDevice(), shaderCreateInfo);
        if (!pointLightShadowShader.isInitialized())
        {
            FL_CORE_ERROR("[Vulkan] Point-light shadow shader failed reflection; shadow pipeline creation skipped");
            return;
        }

        m_PointLightShadowMaps.CreatePipeline(pointLightShadowShader);
    }
}

void vk::backend::impl::updatePointLight(const SFLPointLight* light, uint32_t lightCount)
{
    m_PointLights.clear();
    if (lightCount == 0)
        return;

    assert(light != nullptr);
    m_PointLights.reserve(lightCount);
    m_PointLights.assign(light, light + lightCount);

    assert(m_PointLights.size() <= m_MaxPointLights);

    m_PointLightsBuffer->UploadDataToBuffer(light, lightCount);
}

std::vector<Fleur::Vec4> vk::backend::impl::getFrustumCornersWorldSpace(const Fleur::Mat4& proj, const Fleur::Mat4& view)
{
    const auto inv = Fleur::Math::inverse(proj * view);

    std::vector<Fleur::Vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                const Fleur::Vec4 pt = inv * Fleur::Vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, z, 1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }

    return frustumCorners;
}
std::array<Fleur::Vec4, 8> vk::backend::impl::SplitFrustum(const std::vector<Fleur::Vec4>& corners, float splitNear, float splitFar)
{
    std::array<Fleur::Vec4, 8> result{};

    const int nearIndices[4] = {0, 2, 4, 6};
    const int farIndices[4] = {1, 3, 5, 7};

    for (int i = 0; i < 4; ++i)
    {
        const auto& nearCorner = corners[nearIndices[i]];
        const auto& farCorner = corners[farIndices[i]];

        result[nearIndices[i]] = nearCorner + (farCorner - nearCorner) * splitNear;

        result[farIndices[i]] = nearCorner + (farCorner - nearCorner) * splitFar;
    }

    return result;
}

void vk::backend::impl::updateShadowMapDescriptorSets()
{
    for (size_t i = 0; i < m_Frames.size(); ++i)
    {
        vk::abstraction::DescriptorWriter shadowMapWriter{};
        shadowMapWriter.write_image(0, m_Frames[i].m_DirectionalLightShadowMap.GetImageView(), m_ShadowMapSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        shadowMapWriter.write_buffer(1, m_Frames[i].scene.m_DirectionalShadowMatricesBuffer.GetBuffer(), sizeof(DirectionalShadowMatrices), 0,
                                     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        auto& frame = m_Frames[i];
        shadowMapWriter.update_set(m_Device->GetLogicalDevice(), frame.scene.m_ShadowMapDescriptorSet);
    }
}

void vk::backend::impl::BeginRendering(VkCommandBuffer cmd, const FBeginRenderingDesc& desc)
{
    const uint32_t colorAttachmentCount = desc.colorAttachmentCount != 0 ? desc.colorAttachmentCount : (desc.colorAttachment ? 1u : 0u);
    std::vector<VkRenderingAttachmentInfoKHR> colorAttachmentInfos(colorAttachmentCount);
    VkRenderingAttachmentInfoKHR depthAttachmentInfo{};

    const VkRenderingAttachmentInfoKHR* pColorAttachments = colorAttachmentInfos.data();
    const VkRenderingAttachmentInfoKHR* pDepthAttachment = nullptr;

    for (uint32_t i = 0; i < colorAttachmentCount; ++i)
    {
        const FRenderingColorAttachmentDesc& colorDesc = desc.colorAttachment[i];
        VkRenderingAttachmentInfoKHR& colorInfo = colorAttachmentInfos[i];
        colorInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorInfo.imageView = colorDesc.imageView;
        colorInfo.imageLayout = colorDesc.imageLayout;
        colorInfo.resolveMode = colorDesc.resolveMode;
        colorInfo.resolveImageView = colorDesc.resolveImageView;
        colorInfo.resolveImageLayout = colorDesc.resolveImageLayout;
        colorInfo.loadOp = colorDesc.loadOp;
        colorInfo.storeOp = colorDesc.storeOp;
        colorInfo.clearValue = colorDesc.clearValue;
    }

    if (desc.depthAttachment)
    {
        depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        depthAttachmentInfo.pNext = nullptr;
        depthAttachmentInfo.imageView = desc.depthAttachment->imageView;
        depthAttachmentInfo.imageLayout = desc.depthAttachment->imageLayout;
        depthAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
        depthAttachmentInfo.resolveImageView = VK_NULL_HANDLE;
        depthAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachmentInfo.loadOp = desc.depthAttachment->loadOp;
        depthAttachmentInfo.storeOp = desc.depthAttachment->storeOp;
        depthAttachmentInfo.clearValue = desc.depthAttachment->clearValue;

        pDepthAttachment = &depthAttachmentInfo;
    }

    VkRenderingInfoKHR renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderingInfo.pNext = nullptr;
    renderingInfo.flags = 0;
    renderingInfo.renderArea = desc.renderArea;
    renderingInfo.layerCount = desc.layerCount;
    renderingInfo.viewMask = desc.viewMask;
    renderingInfo.colorAttachmentCount = colorAttachmentCount;
    renderingInfo.pColorAttachments = pColorAttachments;
    renderingInfo.pDepthAttachment = pDepthAttachment;
    renderingInfo.pStencilAttachment = nullptr;

    vkCmdBeginRendering(cmd, &renderingInfo);
}

void vk::backend::impl::BeginShadowRendering()
{
    auto& frame = GetCurrentFrame();
    auto& cmd = frame.m_CommandBuffers;
    auto& directionalLightShadowMap = frame.m_DirectionalLightShadowMap;

    transitionImageLayout(*cmd.GetCommandBuffer(), directionalLightShadowMap.GetImage(), FVkDepthTarget::FindDepthFormat(m_Device->GetPhysicalDevice()),
                          frame.m_DirectionalLightShadowMapLayout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 1,
                          m_CascadeCount);
    frame.m_DirectionalLightShadowMapLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    const VkExtent2D shadowExtent = directionalLightShadowMap.GetExtent();
    VkRect2D renderArea{
        .offset = {0, 0},
        .extent = shadowExtent,
    };

    FRenderingDepthAttachmentDesc depthDesc{};
    // GetImageView() returns the layered VK_IMAGE_VIEW_TYPE_2D_ARRAY view used by the cascade shadow map.
    depthDesc.imageView = directionalLightShadowMap.GetImageView();
    depthDesc.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthDesc.clearValue.depthStencil = {.depth = 1.0f, .stencil = 0};

    FBeginRenderingDesc renderingDesc{};
    renderingDesc.renderArea = renderArea;

    // No color attachment for shadow map.
    renderingDesc.colorAttachment = nullptr;
    renderingDesc.depthAttachment = &depthDesc;

    renderingDesc.layerCount = m_CascadeCount;
    renderingDesc.viewMask = 0;

    BeginRendering(*cmd.GetCommandBuffer(), renderingDesc);

    cmd.BindVertexBuffer(&m_VertexBuffer->GetBuffer());
    cmd.BindIndexBuffer(&m_IndexBuffer->GetBuffer(), VK_INDEX_TYPE_UINT32);

    // We need to bind shadow pass pipeline
    cmd.BindPipeline(m_ShadowPipeline->GetPipeline());

    VkViewport defaultViewport{.x = 0, .y = 0, .width = (float)shadowExtent.width, .height = (float)shadowExtent.height, .minDepth = 0, .maxDepth = 1.0f};
    cmd.SetViewport(defaultViewport);

    VkRect2D defaultScissors{
        .offset = VkOffset2D{.x = 0, .y = 0},
        .extent = shadowExtent,
    };
    cmd.SetScissors(defaultScissors);
}

vk::backend::impl::DirectionalLightShadowFrustum vk::backend::impl::BuildDirectionalShadowFrustum(const std::array<Fleur::Vec4, 8>& corners,
                                                                                                  const Fleur::Vec3& lightDirection,
                                                                                                  const std::array<Fleur::Vec4, 8>* casterCorners,
                                                                                                  size_t cascadeIndex) const
{
    DirectionalLightShadowFrustum result{};

    Fleur::Vec3 up(0.0f, 1.0f, 0.0f);
    if (Fleur::Math::abs(Fleur::Math::dot(lightDirection, up)) > 0.99f)
        up = Fleur::Vec3(1.0f, 0.0f, 0.0f);

    for (const Fleur::Vec4& corner : corners) result.center += Fleur::Vec3(corner);
    result.center /= static_cast<float>(corners.size());

    for (const Fleur::Vec4& corner : corners)
    {
        result.radius = std::max(result.radius, Fleur::Math::length(Fleur::Vec3(corner) - result.center));
    }

    // Keep the light-space basis fixed while the camera moves. Building the
    // view around result.center makes the entire shadow grid follow the camera
    // and defeats texel snapping. The anchor is a stable world-space point;
    // only the orthographic center below follows the cascade bounds.
    const Fleur::Vec3 lightAnchor = m_ShadowMapFrustumSettings.center;
    float lightDistance = std::max(100.0f, m_ShadowMapFrustumSettings.farExtension + m_ShadowMapFrustumSettings.halfSize);
    if (m_HasShadowSceneBounds)
    {
        const Fleur::Vec3 sceneExtent = m_ShadowSceneBounds.GetMax() - m_ShadowSceneBounds.GetMin();
        lightDistance = std::max(lightDistance, Fleur::Math::length(sceneExtent) + 10.0f);
    }
    const Fleur::Vec3 lightPos = lightAnchor - lightDirection * lightDistance;
    const Fleur::Mat4 lightView = Fleur::Math::lookAt(lightPos, lightAnchor, up);

    float minX = std::numeric_limits<float>::infinity();
    float maxX = -std::numeric_limits<float>::infinity();
    float minY = std::numeric_limits<float>::infinity();
    float maxY = -std::numeric_limits<float>::infinity();
    float minZ = std::numeric_limits<float>::infinity();
    float maxZ = -std::numeric_limits<float>::infinity();

    const auto accumulateLightSpaceBounds = [&](const auto& points)
    {
        for (const Fleur::Vec4& corner : points)
        {
            const Fleur::Vec3 lightSpaceCorner = Fleur::Vec3(lightView * corner);
            minX = std::min(minX, lightSpaceCorner.x);
            maxX = std::max(maxX, lightSpaceCorner.x);
            minY = std::min(minY, lightSpaceCorner.y);
            maxY = std::max(maxY, lightSpaceCorner.y);
            minZ = std::min(minZ, lightSpaceCorner.z);
            maxZ = std::max(maxZ, lightSpaceCorner.z);
        }
    };
    accumulateLightSpaceBounds(corners);
    if (casterCorners)
        accumulateLightSpaceBounds(*casterCorners);

    if (!std::isfinite(minX) || !std::isfinite(maxX) || !std::isfinite(minY) || !std::isfinite(maxY) || !std::isfinite(minZ) || !std::isfinite(maxZ))
    {
        return result;
    }

    const float cascadeNear = std::max(0.01f, -maxZ);
    const float cascadeFar = std::max(cascadeNear + 0.1f, -minZ);
    constexpr float depthPadding = 2.0f;
    const float shadowNear = std::max(0.01f, cascadeNear - depthPadding);
    const float shadowFar = std::max(shadowNear + 0.1f, cascadeFar + depthPadding);

    float halfWidth = std::max((maxX - minX) * 0.5f, 0.01f);
    float halfHeight = std::max((maxY - minY) * 0.5f, 0.01f);
    if (cascadeIndex < m_CascadeCount)
    {
        m_StableCascadeHalfWidths[cascadeIndex] = std::max(m_StableCascadeHalfWidths[cascadeIndex], halfWidth);
        m_StableCascadeHalfHeights[cascadeIndex] = std::max(m_StableCascadeHalfHeights[cascadeIndex], halfHeight);
        halfWidth = m_StableCascadeHalfWidths[cascadeIndex];
        halfHeight = m_StableCascadeHalfHeights[cascadeIndex];
    }
    // One texel of guard band prevents nearest-texel center snapping from
    // moving an original bound just outside the orthographic projection.
    halfWidth += (2.0f * halfWidth) / static_cast<float>(kDirectionalShadowMapResolution);
    halfHeight += (2.0f * halfHeight) / static_cast<float>(kDirectionalShadowMapResolution);
    const float texelSizeX = (2.0f * halfWidth) / static_cast<float>(kDirectionalShadowMapResolution);
    const float texelSizeY = (2.0f * halfHeight) / static_cast<float>(kDirectionalShadowMapResolution);
    const float centerX = Fleur::ShadowMath::SnapToTexel((minX + maxX) * 0.5f, texelSizeX);
    const float centerY = Fleur::ShadowMath::SnapToTexel((minY + maxY) * 0.5f, texelSizeY);

    Fleur::Mat4 lightProjection =
        Fleur::Math::orthoRH_ZO(centerX - halfWidth, centerX + halfWidth, centerY - halfHeight, centerY + halfHeight, shadowNear, shadowFar);
    // Flip the complete NDC Y row for Vulkan. The translation term must be
    // flipped together with the scale when the ortho box is not centered at 0.
    lightProjection[1][1] *= -1;
    lightProjection[3][1] *= -1;
    result.lightSpaceMatrix = lightProjection * lightView;
    return result;
}

void vk::backend::impl::UpdateDirectionalShadowFrustum(const Fleur::Vec3& lightDirection)
{
    if (!m_HasLastDirectionalLightDirection || Fleur::Math::abs(lightDirection.x - m_LastDirectionalLightDirection.x) > 0.0001f ||
        Fleur::Math::abs(lightDirection.y - m_LastDirectionalLightDirection.y) > 0.0001f ||
        Fleur::Math::abs(lightDirection.z - m_LastDirectionalLightDirection.z) > 0.0001f)
    {
        m_LastDirectionalLightDirection = lightDirection;
        m_HasLastDirectionalLightDirection = true;
        m_StableCascadeHalfWidths.fill(0.0f);
        m_StableCascadeHalfHeights.fill(0.0f);
    }

    const Fleur::Vec3 sceneMin = m_ShadowSceneBounds.GetMin();
    const Fleur::Vec3 sceneMax = m_ShadowSceneBounds.GetMax();
    const bool validSceneBounds = m_HasShadowSceneBounds && std::isfinite(sceneMin.x) && std::isfinite(sceneMin.y) && std::isfinite(sceneMin.z) &&
                                  std::isfinite(sceneMax.x) && std::isfinite(sceneMax.y) && std::isfinite(sceneMax.z) && sceneMin.x < sceneMax.x &&
                                  sceneMin.y < sceneMax.y && sceneMin.z < sceneMax.z;

    std::array<Fleur::Vec4, 8> sceneCorners{};
    if (validSceneBounds)
    {
        sceneCorners = {Fleur::Vec4(sceneMin.x, sceneMin.y, sceneMin.z, 1.0f), Fleur::Vec4(sceneMax.x, sceneMin.y, sceneMin.z, 1.0f),
                        Fleur::Vec4(sceneMax.x, sceneMax.y, sceneMin.z, 1.0f), Fleur::Vec4(sceneMin.x, sceneMax.y, sceneMin.z, 1.0f),
                        Fleur::Vec4(sceneMin.x, sceneMin.y, sceneMax.z, 1.0f), Fleur::Vec4(sceneMax.x, sceneMin.y, sceneMax.z, 1.0f),
                        Fleur::Vec4(sceneMax.x, sceneMax.y, sceneMax.z, 1.0f), Fleur::Vec4(sceneMin.x, sceneMax.y, sceneMax.z, 1.0f)};
    }
    else
    {
        const Fleur::Vec3 cameraPosition = Fleur::Vec3(Fleur::Math::inverse(m_FrameData.camera.view)[3]);
        const float halfSize = std::max(m_ShadowMapFrustumSettings.halfSize, 1.0f);
        const float shadowDistance = std::max(m_ShadowMapFrustumSettings.farExtension, m_FrameData.camera.farPlane);
        const Fleur::Vec3 baseMin = cameraPosition - Fleur::Vec3(halfSize);
        const Fleur::Vec3 baseMax = cameraPosition + Fleur::Vec3(halfSize);
        const Fleur::Vec3 lightExtrusion = lightDirection * shadowDistance;
        const Fleur::Vec3 extrudedMin = baseMin + lightExtrusion;
        const Fleur::Vec3 extrudedMax = baseMax + lightExtrusion;
        const Fleur::Vec3 casterMin(std::min(baseMin.x, extrudedMin.x), std::min(baseMin.y, extrudedMin.y), std::min(baseMin.z, extrudedMin.z));
        const Fleur::Vec3 casterMax(std::max(baseMax.x, extrudedMax.x), std::max(baseMax.y, extrudedMax.y), std::max(baseMax.z, extrudedMax.z));
        sceneCorners = {Fleur::Vec4(casterMin.x, casterMin.y, casterMin.z, 1.0f), Fleur::Vec4(casterMax.x, casterMin.y, casterMin.z, 1.0f),
                        Fleur::Vec4(casterMax.x, casterMax.y, casterMin.z, 1.0f), Fleur::Vec4(casterMin.x, casterMax.y, casterMin.z, 1.0f),
                        Fleur::Vec4(casterMin.x, casterMin.y, casterMax.z, 1.0f), Fleur::Vec4(casterMax.x, casterMin.y, casterMax.z, 1.0f),
                        Fleur::Vec4(casterMax.x, casterMax.y, casterMax.z, 1.0f), Fleur::Vec4(casterMin.x, casterMax.y, casterMax.z, 1.0f)};
    }

    m_CascadeSplits.Update(m_FrameData.camera.nearPlane, m_FrameData.camera.farPlane, 1.0f, m_CascadeCount);

    const auto cameraFrustumInWorld = getFrustumCornersWorldSpace(m_FrameData.camera.proj, m_FrameData.camera.view);
    float previousSplit = 0.0f;
    for (size_t i = 0; i < m_CascadeCount; ++i)
    {
        const float currentSplit = m_CascadeSplits.splits[i];
        const auto cascadeCorners = SplitFrustum(cameraFrustumInWorld, previousSplit, currentSplit);
        m_CascadeShadowFrustums[i] = BuildDirectionalShadowFrustum(cascadeCorners, lightDirection, &sceneCorners, i);
        previousSplit = currentSplit;
    }
}

void vk::backend::impl::ExecuteDirectionalShadowSubpass()
{
    auto& frame = GetCurrentFrame();
    auto& cmd = frame.m_CommandBuffers;

    const Fleur::Vec3 lightDirection = Fleur::Math::normalize(Fleur::Vec3(m_FrameData.directionalLight.dirIntens));
    UpdateDirectionalShadowFrustum(lightDirection);

    DirectionalShadowMatrices matrices{};
    matrices.cascadeCount = m_CascadeCount;
    for (size_t i = 0; i < m_CascadeCount; ++i) matrices.lightSpaceMatrices[i] = m_CascadeShadowFrustums[i].lightSpaceMatrix;
    const float cameraDepthRange = m_FrameData.camera.farPlane - m_FrameData.camera.nearPlane;
    for (size_t i = 0; i < m_CascadeCount; ++i)
        matrices.cascadeSplits[i / 4][i % 4] = m_FrameData.camera.nearPlane + m_CascadeSplits.splits[i] * cameraDepthRange;
    frame.scene.m_DirectionalShadowMatricesBuffer.MemCopy(&matrices, sizeof(matrices));

    std::array<VkDescriptorSet, 2> shadowDescriptorSets{
        frame.scene.m_SceneNodeTransformsDescriptor,
        frame.scene.m_DirectionalShadowMatricesDescriptor,
    };
    cmd.BindDescriptorSets(m_ShadowPipeline->GetPipelineLayout(), shadowDescriptorSets.data(), shadowDescriptorSets.size());

    for (const auto& drawItem : m_OpaqueDrawItems)
    {
        const auto& primitive = m_Primitives[drawItem.primitiveIdx];
        struct ShadowPushConstant
        {
            uint32_t modelIdx;
            uint32_t nodeIdx;
        } pc;
        pc.modelIdx = drawItem.modelTransformIdx;
        pc.nodeIdx = drawItem.nodeTransformsStartIdx;
        cmd.PushConstant(m_ShadowPipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, pc);
        cmd.DrawIndexed(primitive.indexCount, primitive.indexOffset, primitive.vertexOffset, drawItem.instanceCount, 0);
    }

    cmd.EndRendering();
}

void vk::backend::impl::ExecutePointLightShadowSubpass()
{
    auto& frame = GetCurrentFrame();
    auto& cmd = frame.m_CommandBuffers;

    std::array<Fleur::Mat4, kPointLightShadowFaceCount> faceViewProjections{};
    const std::array<Fleur::Vec3, kPointLightShadowFaceCount> faceDirections{Fleur::Vec3(+1, 0, 0), Fleur::Vec3(-1, 0, 0), Fleur::Vec3(0, +1, 0),
                                                                             Fleur::Vec3(0, -1, 0), Fleur::Vec3(0, 0, +1), Fleur::Vec3(0, 0, -1)};
    const std::array<Fleur::Vec3, kPointLightShadowFaceCount> faceUps{Fleur::Vec3(0, -1, 0), Fleur::Vec3(0, -1, 0), Fleur::Vec3(0, 0, +1),
                                                                      Fleur::Vec3(0, 0, -1), Fleur::Vec3(0, -1, 0), Fleur::Vec3(0, -1, 0)};

    m_PointLightShadowMaps.PrepareForSampling(cmd);

    const size_t shadowedLightCount = std::min(m_PointLights.size(), static_cast<size_t>(m_PointLightShadowMaps.GetTextureCount()));
    for (size_t i = 0; i < shadowedLightCount; i++)
    {
        const float shadowFar = m_PointLights[i].radius;
        Fleur::Mat4 shadowProj = Fleur::Math::perspective(Fleur::Math::radians(90.0f), 1.0f, kPointLightShadowNear, shadowFar);
        shadowProj[1][1] *= -1;

        for (size_t j = 0; j < kPointLightShadowFaceCount; j++)
        {
            faceViewProjections[j] = shadowProj * Fleur::Math::lookAt(Fleur::Vec3(m_PointLights[i].pos), m_PointLights[i].pos + faceDirections[j], faceUps[j]);
        }
        m_PointLightShadowMaps.UpdateMatrices(static_cast<uint32_t>(i), faceViewProjections);
        m_PointLightShadowMaps.Begin(cmd, i);

        cmd.BindVertexBuffer(&m_VertexBuffer->GetBuffer());
        cmd.BindIndexBuffer(&m_IndexBuffer->GetBuffer(), VK_INDEX_TYPE_UINT32);
        cmd.BindDescriptorSets(m_PointLightShadowMaps.GetPipelineLayout(), &frame.scene.m_SceneNodeTransformsDescriptor, 1);

        for (const auto& drawItem : m_OpaqueDrawItems)
        {
            const auto& primitive = m_Primitives[drawItem.primitiveIdx];
            const PointLightShadowMap::PushConstant pc = m_PointLightShadowMaps.MakePushConstant(drawItem.modelTransformIdx, drawItem.nodeTransformsStartIdx);
            cmd.PushConstant(m_PointLightShadowMaps.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, pc);
            cmd.DrawIndexed(primitive.indexCount, primitive.indexOffset, primitive.vertexOffset, drawItem.instanceCount, 0);
        }

        m_PointLightShadowMaps.End(cmd);
    }
}

void vk::backend::impl::ExecuteShadowPass()
{
    auto& frame = GetCurrentFrame();
    auto& cmd = frame.m_CommandBuffers;
    auto& shadowMap = frame.m_DirectionalLightShadowMap;

    cmd.CmdBeginDebugLabel(vk::myVkCmdBeginDebugUtilsLabelEXT, "Shadow Pass");

    cmd.CmdBeginDebugLabel(vk::myVkCmdBeginDebugUtilsLabelEXT, "Directional Shadow Subpass");
    BeginShadowRendering();
    ExecuteDirectionalShadowSubpass();
    cmd.CmdEndDebugLabel(vk::myVkCmdEndDebugUtilsLabelEXT);

    cmd.CmdBeginDebugLabel(vk::myVkCmdBeginDebugUtilsLabelEXT, "Point Light Shadow Subpass");
    ExecutePointLightShadowSubpass();
    cmd.CmdEndDebugLabel(vk::myVkCmdEndDebugUtilsLabelEXT);

    transitionImageLayout(*cmd.GetCommandBuffer(), shadowMap.GetImage(), FVkDepthTarget::FindDepthFormat(m_Device->GetPhysicalDevice()),
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 1,
                          m_CascadeCount);
    frame.m_DirectionalLightShadowMapLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    cmd.CmdEndDebugLabel(vk::myVkCmdEndDebugUtilsLabelEXT);
}

void vk::backend::impl::ExecuteMainPass()
{
    auto& cmd = GetCurrentFrame().m_CommandBuffers;

    transitionImageLayout(*cmd.GetCommandBuffer(), m_Swapchain->GetSwapchainImage(m_ImageIndex), m_Swapchain->GetImageFormat(),
                          m_SwapchainImageLayouts[m_ImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    m_SwapchainImageLayouts[m_ImageIndex] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkRect2D renderArea{
        .offset = {0, 0},
        .extent = {.width = m_Swapchain->GetSwapchainExtent().width, .height = m_Swapchain->GetSwapchainExtent().height},
    };


    FRenderingColorAttachmentDesc colorDesc{};
    colorDesc.imageView = m_Swapchain->GetSwapchainImageView(m_ImageIndex);
    colorDesc.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorDesc.clearValue.color = {{1.0f, 1.0f, 1.0f, 1.0f}};

    VkSampleCountFlagBits sampleCount = m_MultisampledRenderTarget->GetSamplesCount();

    if (sampleCount > VK_SAMPLE_COUNT_1_BIT)
    {
        colorDesc.imageView = m_MultisampledRenderTarget->GetTexture()->GetImageView();
        colorDesc.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
        colorDesc.resolveImageView = m_Swapchain->GetSwapchainImageView(m_ImageIndex);
        colorDesc.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }

    FRenderingDepthAttachmentDesc depthDesc{};
    depthDesc.imageView = m_DepthRenderTarget.GetImageView();
    depthDesc.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthDesc.clearValue.depthStencil = {.depth = 1.0f, .stencil = 0};

    FBeginRenderingDesc renderingDesc{};
    renderingDesc.renderArea = renderArea;
    renderingDesc.colorAttachment = &colorDesc;
    renderingDesc.depthAttachment = &depthDesc;
    renderingDesc.layerCount = 1;
    renderingDesc.viewMask = 0;

    BeginRendering(*cmd.GetCommandBuffer(), renderingDesc);

    if (m_Skybox)
    {
        m_Skybox->Record(*cmd.GetCommandBuffer(), m_Swapchain->GetSwapchainExtent(), m_FrameData.camera);
    }

    if (m_Floor)
    {
        // m_Floor->Record(*cmd.GetCommandBuffer(), GetCurrentFrame().scene.m_SceneDataDescriptor, m_Swapchain->GetSwapchainExtent(), m_FrameData.camera);
    }

    cmd.BindVertexBuffer(&m_VertexBuffer->GetBuffer());
    cmd.BindIndexBuffer(&m_IndexBuffer->GetBuffer(), VK_INDEX_TYPE_UINT32);


    cmd.BindPipeline(m_OpaquePipeline->GetPipeline());

    VkViewport defaultViewport{.x = 0,
                               .y = 0,
                               .width = (float)m_Swapchain->GetSwapchainExtent().width,
                               .height = (float)m_Swapchain->GetSwapchainExtent().height,
                               .minDepth = 0,
                               .maxDepth = 1.0f};
    cmd.SetViewport(defaultViewport);

    VkRect2D defaultScissors{
        .offset = VkOffset2D{.x = 0, .y = 0},
        .extent = m_Swapchain->GetSwapchainExtent(),
    };
    cmd.SetScissors(defaultScissors);

    auto& frame = GetCurrentFrame();
    const auto& shadowMap = frame.m_DirectionalLightShadowMap;
    std::array<VkDescriptorSet, 7> descriptorSets{
        frame.scene.m_SceneDataDescriptor,    m_TextureDescriptorSet,         frame.scene.m_SceneNodeTransformsDescriptor, m_PointLightsDescriptorSet,
        frame.scene.m_ShadowMapDescriptorSet, m_ShadowMapOffsetDescriptorSet, m_PointLightShadowMapsDescriptorSet};

    cmd.CmdBeginDebugLabel(vk::myVkCmdBeginDebugUtilsLabelEXT, "Opaque Pass");
    updateTextureDescriptorSet(m_TextureDescriptorSet, kDebugShadowMapTextureSlot, shadowMap.GetImageView(), m_ShadowMapSampler);
    cmd.BindDescriptorSets(m_OpaquePipelineLayout->Get(), descriptorSets.data(), descriptorSets.size());

    for (const auto& drawItem : m_OpaqueDrawItems)
    {
        const auto& primitive = m_Primitives[drawItem.primitiveIdx];
        SFLDrawPushConstants pushConstants = MakeDrawPushConstants(primitive);
        pushConstants.drawIndices.x = drawItem.nodeTransformsStartIdx;
        pushConstants.drawIndices.y = drawItem.modelTransformIdx;
        pushConstants.drawIndices.w = m_PointLights.size();
        pushConstants.materialParams.y = static_cast<float>(m_DirectionalLightSampling);
        pushConstants.materialParams.z = static_cast<float>(m_PointLightSampling);
        pushConstants.materialParams.w = m_NormalMappingEnabled ? 1.0f : 0.0f;
        cmd.PushConstant(m_OpaquePipelineLayout->Get(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstants);
        cmd.DrawIndexed(primitive.indexCount, primitive.indexOffset, primitive.vertexOffset, drawItem.instanceCount, 0);
    }

    cmd.BindPipeline(m_TransparentPipeline->GetPipeline());
    for (const auto& drawItem : m_TransparentDrawItems)
    {
        const auto& primitive = m_Primitives[drawItem.primitiveIdx];
        SFLDrawPushConstants pushConstants = MakeDrawPushConstants(primitive);
        pushConstants.drawIndices.x = drawItem.nodeTransformsStartIdx;
        pushConstants.drawIndices.y = drawItem.modelTransformIdx;
        pushConstants.drawIndices.w = m_PointLights.size();
        pushConstants.materialParams.y = static_cast<float>(m_DirectionalLightSampling);
        pushConstants.materialParams.z = static_cast<float>(m_PointLightSampling);
        pushConstants.materialParams.w = m_NormalMappingEnabled ? 1.0f : 0.0f;
        cmd.PushConstant(m_OpaquePipelineLayout->Get(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstants);
        cmd.DrawIndexed(primitive.indexCount, primitive.indexOffset, primitive.vertexOffset, drawItem.instanceCount, 0);
    }
    cmd.CmdEndDebugLabel(vk::myVkCmdEndDebugUtilsLabelEXT);

    ClearFrameDrawItems();

    cmd.CmdBeginDebugLabel(vk::myVkCmdBeginDebugUtilsLabelEXT, "Overlay Pass");
    m_OverlayPass->Record(cmd, m_CurrentFrame);
    m_OverlayPass->Clear();
    cmd.CmdEndDebugLabel(vk::myVkCmdEndDebugUtilsLabelEXT);

    cmd.CmdBeginDebugLabel(vk::myVkCmdBeginDebugUtilsLabelEXT, "Debug Pass");
    m_DebugDraw->RecordWorld(cmd, m_FrameData.camera, m_CurrentFrame);
    cmd.CmdEndDebugLabel(vk::myVkCmdEndDebugUtilsLabelEXT);
    m_DebugDraw->Clear();

    cmd.EndRendering();
    transitionImageLayout(*cmd.GetCommandBuffer(), m_Swapchain->GetSwapchainImage(m_ImageIndex), m_Swapchain->GetImageFormat(),
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    m_SwapchainImageLayouts[m_ImageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    cmd.End();
}

void vk::backend::impl::SubmitFrame()
{
    auto& frame = GetCurrentFrame();
    auto& cmd = frame.m_CommandBuffers;

    VkSemaphore waitSemaphores[] = {frame.m_ImagesAvailable};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = cmd.GetCommandBuffer();

    VkSemaphore signalSemaphores[] = {m_RenderFinished[m_ImageIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    VK_CHECK(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, frame.m_InFlightFences));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.pResults = nullptr;

    VkSwapchainKHR swapChains[] = {m_Swapchain->GetSwapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_ImageIndex;
    if (m_SwapchainImageLayouts[m_ImageIndex] != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        FL_CORE_ERROR("[Vulkan][SwapchainLayout] Present image={} requires PRESENT_SRC_KHR but trackedLayout={}", m_ImageIndex,
                      static_cast<uint32_t>(m_SwapchainImageLayouts[m_ImageIndex]));
    }
    vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);

    m_CurrentFrame = (m_CurrentFrame + 1) % m_FramesInFlight;
}

void vk::backend::impl::initializeOpaqueDescriptorSets()
{
    if (m_OpaqueDescriptorSetsInitialized)
        return;

    assert(m_OpaquePipelineLayout != nullptr);

    createTextureDescriptorPool();
    createTextureDescriptorSets();

    for (Frame& frame : m_Frames)
    {
        frame.scene.m_SceneNodeTransformsDescriptor = frame.frameDescriptors.allocate(m_Device->GetLogicalDevice(), m_OpaquePipelineLayout->GetSetLayout(2), 1);
        frame.scene.m_SceneDataDescriptor = frame.frameDescriptors.allocate(m_Device->GetLogicalDevice(), m_OpaquePipelineLayout->GetSetLayout(0), 1);
        frame.scene.m_ShadowMapDescriptorSet = frame.frameDescriptors.allocate(m_Device->GetLogicalDevice(), m_OpaquePipelineLayout->GetSetLayout(4), 1);

        vk::abstraction::DescriptorWriter ssboWriter{};
        ssboWriter.write_buffer(0, frame.scene.m_SceneNodeTransformsStorageBuffer.GetBuffer(), sizeof(Fleur::Graphics::SFLSSBODescriptorBuffer), 0,
                                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        ssboWriter.update_set(m_Device->GetLogicalDevice(), frame.scene.m_SceneNodeTransformsDescriptor);

        vk::abstraction::DescriptorWriter cameraUboWriter{};
        cameraUboWriter.write_buffer(0, frame.scene.m_SceneDataBuffer.GetBuffer(), sizeof(Fleur::Graphics::SFLSceneDataUBO), 0,
                                     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        cameraUboWriter.update_set(m_Device->GetLogicalDevice(), frame.scene.m_SceneDataDescriptor);
    }

    updateShadowMapDescriptorSets();
    updateTextureDescriptorSet(m_TextureDescriptorSet, m_FallbackTextureIdx, m_TextureMap[m_FallbackTextureIdx].GetImageView(), m_ImageSampler);
    updateTextureDescriptorSet(m_TextureDescriptorSet, kDebugShadowMapTextureSlot, m_Frames.front().m_DirectionalLightShadowMap.GetImageView(),
                               m_ShadowMapSampler);
    m_OpaqueDescriptorSetsInitialized = true;
}

bool vk::backend::impl::beginFrame(const Fleur::Graphics::RenderFrameData& frameData)
{
    if (m_WindowResizeIsInProgress)
        return false;

    if (!m_Swapchain->ReadyToPresent())
    {
        vkDeviceWaitIdle(m_Device->GetLogicalDevice());
        vkQueueWaitIdle(m_Device->GetPresentQueue());
        destroyRenderFinishedSemaphores();
        m_Swapchain->Recreate(m_Surface, m_Device->GetPresentQueueFamilyIndex(), m_MultisampledRenderTarget->GetTexture()->GetImageView(),
                              m_DepthRenderTarget.GetImageView());
        m_SwapchainImageLayouts.assign(m_Swapchain->GetSwapchainImageCount(), VK_IMAGE_LAYOUT_UNDEFINED);
        createRenderFinishedSemaphores();
    }
    m_FrameData = frameData;

    Frame& frame = GetCurrentFrame();
    assert(m_CurrentFrame < m_Frames.size());

    VkResult waitForFences = vkWaitForFences(m_Device->GetLogicalDevice(), 1, &frame.m_InFlightFences, VK_TRUE, UINT64_MAX);
    if (waitForFences != VK_SUCCESS)
    {
        if (waitForFences == VK_ERROR_DEVICE_LOST)
        {
            DBG_PRINTM("Fatal: Vulkan device lost. Stop rendering.");
        }

        return false;
    }

    VkResult isSwapchainValid =
        vkAcquireNextImageKHR(m_Device->GetLogicalDevice(), m_Swapchain->GetSwapchain(), UINT64_MAX, frame.m_ImagesAvailable, VK_NULL_HANDLE, &m_ImageIndex);
    if (isSwapchainValid == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return false;
    }
    else if (isSwapchainValid != VK_SUCCESS && isSwapchainValid != VK_SUBOPTIMAL_KHR)
    {
        DBG_PRINTM("Failed to present swap chain image!")
        return false;
    }

    assert(m_ImageIndex < m_RenderFinished.size());
    if (m_ImageIndex >= m_RenderFinished.size())
    {
        DBG_PRINTM("Acquired swap chain image index is outside render-finished semaphore array")
        return false;
    }

    if (!m_OpaquePipeline)
        return false;

    VkResult resetFences = vkResetFences(m_Device->GetLogicalDevice(), 1, &frame.m_InFlightFences);
    if (resetFences != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to vkResetFences!")
        assert(false);
    }

    m_FrameData = frameData;
    m_FrameData.camera.proj[1][1] *= -1;  // Vulkan Y-flip for all VK passes.

    frame.scene.m_SceneNodeTransformsStorageBuffer.Reset();

    m_PointLightsBuffer->Reset();

    Fleur::Graphics::SFLSceneDataUBO sceneData{
        m_FrameData.camera.view,
        m_FrameData.camera.proj,
        Fleur::Math::inverse(m_FrameData.camera.view)[3],
        m_FrameData.directionalLight.color,
        m_FrameData.directionalLight.dirIntens,
    };
    GetCurrentFrame().scene.m_SceneDataBuffer.MemCopy(&sceneData, sizeof(sceneData));

    VkResult resetCmd = vkResetCommandPool(m_Device->GetLogicalDevice(), frame.m_CommandPools.GetCommandPool(), 0);
    if (resetCmd != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to vkResetCommandPool!")
        assert(false);
    }

    auto& cmd = frame.m_CommandBuffers;
    cmd.Begin();

    return true;
}

void vk::backend::impl::endFrame()
{
    ExecuteShadowPass();
    ExecuteGBufferPass();
    ExecuteLightingPass();
    GetCurrentFrame().m_CommandBuffers.End();
    SubmitFrame();
}

void vk::backend::impl::registerModel(AssetID id, const SVertexData* vertices, uint32_t verticesCount, const uint32_t* indices, uint32_t indexCount,
                                      const Fleur::Mat4* transformNodes, uint32_t transformNodesCount, const FLPrimitiveDrawItem* primitives,
                                      uint32_t primitiveCount, const FLInstanceItem* srcInstances, uint32_t instanceCount)
{
    if (m_RegisteredModels.contains(id))
        return;

    uint32_t batchIdx = m_Batches.size();
    uint32_t instanceStartIdx = m_Instances.size();

    m_RegisteredModels[id] = batchIdx;
    auto& registeredBatch = m_Batches.emplace_back();
    registeredBatch.instancesCount = instanceCount;
    registeredBatch.instanceStartIdx = instanceStartIdx;
    registeredBatch.globalNodeStartIdx = m_InstanceNodeTransforms.size();
    registeredBatch.nodeTransformCount = transformNodesCount;

    uint64_t globalIndexOffset = m_IndexBuffer->CurrentSize() / m_IndexBuffer->StrideBytes();
    uint64_t globalVertexOffset = m_VertexBuffer->CurrentSize() / m_VertexBuffer->StrideBytes();

    if (verticesCount > 0 && indexCount > 0)
    {
        uint32_t maxIndex = 0;
        for (uint32_t i = 0; i < indexCount; ++i) maxIndex = std::max(maxIndex, indices[i]);
        FL_CORE_INFO("[Vulkan][ModelUpload] id={} vertices={} indices={} maxIndex={} stride={} firstPos=({}, {}, {}) firstIndex={}", id, verticesCount,
                     indexCount, maxIndex, sizeof(SVertexData), vertices[0].Position.x, vertices[0].Position.y, vertices[0].Position.z, indices[0]);
    }

    uint64_t primitiveStartIdx = m_Primitives.size();

    m_VertexBuffer->UploadDataToBuffer(vertices, verticesCount);
    m_IndexBuffer->UploadDataToBuffer(indices, indexCount);

    m_InstanceNodeTransforms.reserve(m_InstanceNodeTransforms.size() + transformNodesCount);
    m_InstanceNodeTransforms.insert(m_InstanceNodeTransforms.end(), transformNodes, transformNodes + transformNodesCount);

    for (size_t i = 0; i < instanceCount; i++)
    {
        const auto& srcInstance = srcInstances[i];
        auto& registeredInstance = m_Instances.emplace_back();

        registeredInstance.drawCount = srcInstance.drawCount;
        registeredInstance.globalPrimitiveStartIdx = primitiveStartIdx + i;
        registeredInstance.primitiveCount = srcInstance.primitiveCount;
        registeredInstance.globalNodeTransformStartIdx = registeredBatch.globalNodeStartIdx + srcInstance.nodeTransformStartIdx;
    }

    m_Primitives.reserve(m_Primitives.size() + primitiveCount);
    for (uint32_t i = 0; i < primitiveCount; i++)
    {
        const auto& item = primitives[i];
        auto& primitive = m_Primitives.emplace_back();

        primitive.FromMaterial(item.material);

        primitive.indexCount = item.indexCount;
        primitive.indexOffset = globalIndexOffset + item.indexStart;
        primitive.vertexOffset = globalVertexOffset;
        primitive.bucket = item.material.mode;
        primitive.boundingBoxCenter = item.boundingBoxCenter;
        FL_CORE_INFO("[Vulkan][ModelUpload] primitive={} indexStart={} indexCount={} vertexStart={} bbox=({}, {}, {})", i, item.indexStart, item.indexCount,
                     item.vertexStart, item.boundingBoxCenter.x, item.boundingBoxCenter.y, item.boundingBoxCenter.z);
    }
}

void vk::backend::impl::unregisterModel(AssetID id)
{
    m_RegisteredModels.erase(id);
    // TODO: reclaim geometry buffer space (bump allocator has no free; needs a sub-allocator).
}

void vk::backend::impl::drawModel(AssetID id, const Fleur::Mat4& modelTransform)
{
    auto it = m_RegisteredModels.find(id);
    if (it == m_RegisteredModels.end())
        return;

    //  TODO: per-draw nodeTransform via push-constant (needs vertex shader change). Identity for now.
    const auto& batch = m_Batches[it->second];
    const auto& srcInstance = &m_Instances[batch.instanceStartIdx];
    auto& frame = GetCurrentFrame();

    uint32_t matricesCount = batch.nodeTransformCount + 2;
    std::vector<Fleur::Mat4> matrices(matricesCount);

    matrices[0] = modelTransform;
    matrices[1] = Fleur::Mat4(Fleur::Math::transpose(Fleur::Math::inverse(Fleur::Mat3(modelTransform))));

    memcpy(&matrices[2], &m_InstanceNodeTransforms[batch.globalNodeStartIdx], sizeof(Fleur::Mat4) * batch.nodeTransformCount);

    uint32_t ssboCurrentIdx = frame.scene.m_SceneNodeTransformsStorageBuffer.CurrentSize() / frame.scene.m_SceneNodeTransformsStorageBuffer.StrideBytes();
    frame.scene.m_SceneNodeTransformsStorageBuffer.UploadDataToBuffer(matrices.data(), matricesCount);

    for (size_t i = 0; i < batch.instancesCount; i++)
    {
        const auto& instance = srcInstance[i];
        uint32_t localNodeOffset = instance.globalNodeTransformStartIdx - batch.globalNodeStartIdx;
        uint32_t ssboNodeOffset = ssboCurrentIdx + localNodeOffset + 2;

        for (size_t j = 0; j < instance.primitiveCount; j++)
        {
            const auto& primitive = m_Primitives[instance.globalPrimitiveStartIdx + j];
            if (primitive.bucket == FLAlphaMode::FL_OPAQUE || primitive.bucket == FLAlphaMode::FL_MASK)
            {
                auto& drawItem = m_OpaqueDrawItems.emplace_back();
                drawItem.instanceCount = instance.drawCount;
                drawItem.modelTransformIdx = ssboCurrentIdx;
                drawItem.nodeTransformsStartIdx = ssboNodeOffset;
                drawItem.primitiveIdx = instance.globalPrimitiveStartIdx + j;
                drawItem.boundingBoxCenter = primitive.boundingBoxCenter;
            }
            else
            {
                auto& drawItem = m_TransparentDrawItems.emplace_back();
                drawItem.instanceCount = instance.drawCount;
                drawItem.modelTransformIdx = ssboCurrentIdx;
                drawItem.nodeTransformsStartIdx = ssboNodeOffset;
                drawItem.primitiveIdx = instance.globalPrimitiveStartIdx + j;
                drawItem.boundingBoxCenter = primitive.boundingBoxCenter;

                std::sort(m_TransparentDrawItems.begin(), m_TransparentDrawItems.end(),
                          [](const FLFrameDrawItem& a, const FLFrameDrawItem& b) { return a.boundingBoxCenter.z > b.boundingBoxCenter.z; });
            }
        }
    }
}

VkDescriptorSet vk::backend::impl::GetGBufferMaterialDescriptorSet(PrimitiveDrawInfo& primitive, VkDescriptorSetLayout materialSetLayout,
                                                                     VkImageView fallbackImageView)
{
    VkImageView diffuseImageView = fallbackImageView;
    if (primitive.material.albedo >= 0)
    {
        const auto texture = m_TextureMap.find(primitive.material.albedo);
        if (texture != m_TextureMap.end() && texture->second.GetImageView() != VK_NULL_HANDLE)
            diffuseImageView = texture->second.GetImageView();
    }

    const VkImageView specularImageView = fallbackImageView;
    if (primitive.gBufferMaterialDescriptorSet == VK_NULL_HANDLE)
        primitive.gBufferMaterialDescriptorSet = m_GBufferMaterialDescriptors.allocate(m_Device->GetLogicalDevice(), materialSetLayout, 1);

    if (primitive.gBufferDiffuseImageView != diffuseImageView || primitive.gBufferSpecularImageView != specularImageView)
    {
        vk::abstraction::DescriptorWriter materialWriter{};
        materialWriter.write_image(0, diffuseImageView, m_ImageSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        materialWriter.write_image(1, specularImageView, m_ImageSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        materialWriter.update_set(m_Device->GetLogicalDevice(), primitive.gBufferMaterialDescriptorSet);
        primitive.gBufferDiffuseImageView = diffuseImageView;
        primitive.gBufferSpecularImageView = specularImageView;
    }

    return primitive.gBufferMaterialDescriptorSet;
}

void vk::backend::impl::ClearFrameDrawItems()
{
    m_OpaqueDrawItems.clear();
    m_TransparentDrawItems.clear();
}


// GBuffer
vk::backend::impl::GBuffer::GBuffer(VkDevice device, VkPhysicalDevice physicalDevice, FVkMemoryTracker& memoryTracker, uint32_t width, uint32_t height,
                                    VkFormat depthFormat)
    : m_Device(device)
    , m_PhysicalDevice(physicalDevice)
    , m_MemoryTracker(memoryTracker)
    , m_Width(width)
    , m_Height(height)
{
    // Create Textures
    createTexture(position, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
    createTexture(normal, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
    createTexture(albedo, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    createTexture(depth, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

vk::backend::impl::GBuffer::~GBuffer()
{
}

void vk::backend::impl::GBuffer::createTexture(FVkTexture& texture, VkFormat format, VkImageAspectFlags aspect)
{
    uint32_t mipLevels = 1;
    uint32_t layerCount = 1;
    uint32_t arrayLayers = 1;
    uint32_t channels = GetChannelsNumFromFormat(format);

    const VkImageUsageFlags attachmentUsage = (aspect & VK_IMAGE_ASPECT_DEPTH_BIT) != 0 ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                                                                                          : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = VkExtent3D{.width = m_Width, .height = m_Height, .depth = 1},
        .mipLevels = mipLevels,
        .arrayLayers = arrayLayers,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = attachmentUsage | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VkImage vkImage = texture.CreateImage(m_Device, m_PhysicalDevice, m_MemoryTracker, FVkAllocationCategory::RenderTarget, imageInfo,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, aspect);

    texture.CreateImageView();
}

void vk::backend::impl::ExecuteGBufferPass()
{
    auto& frame = GetCurrentFrame();
    auto& cmd = frame.m_CommandBuffers;

    cmd.CmdBeginDebugLabel(vk::myVkCmdBeginDebugUtilsLabelEXT, "Deferred Rendering");
    cmd.CmdBeginDebugLabel(vk::myVkCmdBeginDebugUtilsLabelEXT, "G-Buffer Pass");

    std::array<FRenderingColorAttachmentDesc, 3> colorAttachments{};
    for (size_t i = 0; i < 3; i++)
    {
        colorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachments[i].clearValue.color = {{1.0f, 1.0f, 1.0f, 1.0f}};
    }
    colorAttachments[0].imageView = m_GBuffer->GetPositionTexture().GetImageView();
    colorAttachments[1].imageView = m_GBuffer->GetNormalTexture().GetImageView();
    colorAttachments[2].imageView = m_GBuffer->GetAlbedoTexture().GetImageView();

    const VkImageLayout previousColorLayout = m_GBuffer->GetColorLayout();
    transitionImageLayout(*cmd.GetCommandBuffer(), m_GBuffer->GetPositionTexture().GetImage(), VK_FORMAT_R16G16B16A16_SFLOAT, previousColorLayout,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    transitionImageLayout(*cmd.GetCommandBuffer(), m_GBuffer->GetNormalTexture().GetImage(), VK_FORMAT_R16G16B16A16_SFLOAT, previousColorLayout,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    transitionImageLayout(*cmd.GetCommandBuffer(), m_GBuffer->GetAlbedoTexture().GetImage(), VK_FORMAT_R8G8B8A8_UNORM, previousColorLayout,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    FRenderingDepthAttachmentDesc depthDesc{};
    depthDesc.imageView = m_DepthRenderTarget.GetImageView();
    depthDesc.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthDesc.clearValue.depthStencil = {.depth = 1.0f, .stencil = 0};


    FBeginRenderingDesc renderingDesc{};
    renderingDesc.colorAttachmentCount = 3;
    renderingDesc.renderArea = m_GBuffer->GetRenderArea();
    renderingDesc.colorAttachment = colorAttachments.data();
    renderingDesc.depthAttachment = &depthDesc;
    renderingDesc.layerCount = 1;
    renderingDesc.viewMask = 0;

    BeginRendering(*cmd.GetCommandBuffer(), renderingDesc);

    cmd.BindVertexBuffer(&m_VertexBuffer->GetBuffer());
    cmd.BindIndexBuffer(&m_IndexBuffer->GetBuffer(), VK_INDEX_TYPE_UINT32);

    // We need to bind shadow pass pipeline
    cmd.BindPipeline(m_GBufferPipeline->GetPipeline());

    VkViewport defaultViewport{
        .x = 0, .y = 0, .width = m_GBuffer->GetViewport().width, .height = m_GBuffer->GetViewport().height, .minDepth = 0, .maxDepth = 1.0f};
    cmd.SetViewport(defaultViewport);

    VkRect2D defaultScissors{
        .offset = VkOffset2D{.x = 0, .y = 0},
        .extent = m_GBuffer->GetRenderArea().extent,
    };
    cmd.SetScissors(defaultScissors);

    const VkImageView fallbackImageView = m_TextureMap[m_FallbackTextureIdx].GetImageView();
    const VkDescriptorSetLayout materialSetLayout = m_GBufferPipelineLayout->GetSetLayout(1);

    for (const auto& drawItem : m_OpaqueDrawItems)
    {
        auto& primitive = m_Primitives[drawItem.primitiveIdx];
        const VkDescriptorSet materialDescriptorSet = GetGBufferMaterialDescriptorSet(primitive, materialSetLayout, fallbackImageView);

        std::array<VkDescriptorSet, 3> descriptorSets{
            frame.scene.m_SceneDataDescriptor,
            materialDescriptorSet,
            frame.scene.m_SceneNodeTransformsDescriptor,
        };
        cmd.BindDescriptorSets(m_GBufferPipelineLayout->Get(), descriptorSets.data(), descriptorSets.size());

        SFLDrawPushConstants pushConstants = MakeDrawPushConstants(primitive);
        pushConstants.drawIndices.x = drawItem.nodeTransformsStartIdx;
        pushConstants.drawIndices.y = drawItem.modelTransformIdx;
        cmd.PushConstant(m_GBufferPipelineLayout->Get(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstants);
        cmd.DrawIndexed(primitive.indexCount, primitive.indexOffset, primitive.vertexOffset, drawItem.instanceCount, 0);
    }

    cmd.EndRendering();

    transitionImageLayout(*cmd.GetCommandBuffer(), m_GBuffer->GetPositionTexture().GetImage(), VK_FORMAT_R16G16B16A16_SFLOAT,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    transitionImageLayout(*cmd.GetCommandBuffer(), m_GBuffer->GetNormalTexture().GetImage(), VK_FORMAT_R16G16B16A16_SFLOAT,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    transitionImageLayout(*cmd.GetCommandBuffer(), m_GBuffer->GetAlbedoTexture().GetImage(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    m_GBuffer->SetColorLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    cmd.CmdEndDebugLabel(vk::myVkCmdEndDebugUtilsLabelEXT);
}

void vk::backend::impl::ExecuteLightingPass()
{
    auto& frame = GetCurrentFrame();
    auto& cmd = frame.m_CommandBuffers;

    cmd.CmdBeginDebugLabel(vk::myVkCmdBeginDebugUtilsLabelEXT, "Lighting Pass");

    transitionImageLayout(*cmd.GetCommandBuffer(), m_Swapchain->GetSwapchainImage(m_ImageIndex), m_Swapchain->GetImageFormat(),
                          m_SwapchainImageLayouts[m_ImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    m_SwapchainImageLayouts[m_ImageIndex] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    FRenderingColorAttachmentDesc colorDesc{};
    colorDesc.imageView = m_Swapchain->GetSwapchainImageView(m_ImageIndex);
    colorDesc.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorDesc.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    const VkRect2D renderArea{.offset = {0, 0}, .extent = m_Swapchain->GetSwapchainExtent()};
    FBeginRenderingDesc renderingDesc{};
    renderingDesc.renderArea = renderArea;
    renderingDesc.colorAttachment = &colorDesc;
    renderingDesc.layerCount = 1;
    BeginRendering(*cmd.GetCommandBuffer(), renderingDesc);

    cmd.BindPipeline(m_DeferredLightingPipeline->GetPipeline());
    const VkViewport viewport{.x = 0.0f,
                              .y = 0.0f,
                              .width = static_cast<float>(renderArea.extent.width),
                              .height = static_cast<float>(renderArea.extent.height),
                              .minDepth = 0.0f,
                              .maxDepth = 1.0f};
    cmd.SetViewport(viewport);
    cmd.SetScissors(renderArea);

    std::array<VkDescriptorSet, 5> descriptorSets{frame.scene.m_SceneDataDescriptor, m_GBufferLightingDescriptorSet, m_PointLightsDescriptorSet,
                                                   frame.scene.m_ShadowMapDescriptorSet, m_PointLightShadowMapsDescriptorSet};
    cmd.BindDescriptorSets(m_DeferredLightingPipelineLayout->Get(), descriptorSets.data(), descriptorSets.size());
    cmd.Draw(3);
    cmd.EndRendering();

    transitionImageLayout(*cmd.GetCommandBuffer(), m_Swapchain->GetSwapchainImage(m_ImageIndex), m_Swapchain->GetImageFormat(),
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    m_SwapchainImageLayouts[m_ImageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    cmd.CmdEndDebugLabel(vk::myVkCmdEndDebugUtilsLabelEXT);

    ClearFrameDrawItems();
}
