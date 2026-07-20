#include "Renderer_Vulkan.h"

// This entire .cpp file was so big so it was pain in the ass to navigate throughout
// I've hidden vulkanBackendImpl declaration into .hpp file
#if defined(FLEUR_PLATFORM_WIN)
#define NOMINMAX
#define NOGDI
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "PrivateVulkanImpl.hpp"

namespace
{
constexpr uint32_t kDebugShadowMapTextureSlot = MAX_TEXTURES - 1;
}


// ---------- backend ----------
vk::backend::backend(bool enableValidation, void* pNativeHandle, Fleur::SRect& framebufferSize, Fleur::Graphics::SFLImageView& fallback)
    : pImpl(new vk::backend::impl(enableValidation, pNativeHandle, framebufferSize, fallback))
{
}
vk::backend::~backend()
{
    delete pImpl;
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
    assert(shaderStages.fragment.shaderCode && shaderStages.fragment.sizeBytes > 0);
    assert(shaderStages.vertex.shaderCode && shaderStages.vertex.sizeBytes > 0);

    pImpl->createPass(kind, shaderStages);
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
        vk::ShaderCreateInfo primitivesShaderCreateInfo{.pVertexData = shaders.primitives.vertex.shaderCode,
                                                        .vertexSize = shaders.primitives.vertex.sizeBytes,
                                                        .pFragmentData = shaders.primitives.fragment.shaderCode,
                                                        .fragmentSize = shaders.primitives.fragment.sizeBytes};
        vk::ShaderCreateInfo geometryShaderCreateInfo{.pVertexData = shaders.geometry.vertex.shaderCode,
                                                      .vertexSize = shaders.geometry.vertex.sizeBytes,
                                                      .pFragmentData = shaders.geometry.fragment.shaderCode,
                                                      .fragmentSize = shaders.geometry.fragment.sizeBytes};

        auto& primitivesShader = pImpl->m_ShaderMap.emplace("DebugPrimitives", vk::FVkShader()).first->second;
        if (!primitivesShader.isInitialized())
            primitivesShader.Init(pImpl->m_Device->GetLogicalDevice(), primitivesShaderCreateInfo);

        auto& geometryShader = pImpl->m_ShaderMap.emplace("DebugGeometry", vk::FVkShader()).first->second;
        if (!geometryShader.isInitialized())
            geometryShader.Init(pImpl->m_Device->GetLogicalDevice(), geometryShaderCreateInfo);
        pImpl->m_DebugDraw->Create(pImpl->m_Device, pImpl->m_Swapchain, &primitivesShader, &geometryShader,
                                   pImpl->m_TextureDescriptorSetLayout->GetDescriptorSetLayout(), pImpl->m_TextureDescriptorSet,
                                   pImpl->m_MultisampledRenderTarget->GetSamplesCount(), FVkDepthTarget::FindDepthFormat(pImpl->m_Device->GetPhysicalDevice()),
                                   pImpl->m_FramesInFlight);
    }
}
void vk::backend::ConfigureOverlay(SFLShaderStages shaderStages)
{
    vk::ShaderCreateInfo overlayShaderCreateInfo{.pVertexData = shaderStages.vertex.shaderCode,
                                                 .vertexSize = shaderStages.vertex.sizeBytes,
                                                 .pFragmentData = shaderStages.fragment.shaderCode,
                                                 .fragmentSize = shaderStages.fragment.sizeBytes};

    auto& overlayShader = pImpl->m_ShaderMap.emplace("Overlay", vk::FVkShader()).first->second;
    if (!overlayShader.isInitialized())
        overlayShader.Init(pImpl->m_Device->GetLogicalDevice(), overlayShaderCreateInfo);

    if (!pImpl->m_OverlayPass->IsInitialized())
        return;

    pImpl->m_OverlayPass->SetShader(&overlayShader);
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

void vk::backend::DrawShadowMapOverlay(Fleur::Vec2 min, Fleur::Vec2 max)
{
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
                        Fleur::Graphics::SFLImageView& fallback)
// clang-format on
{
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

    m_Swapchain = new FVkSwapchain();
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

    initializeVma();
    m_Swapchain->CreateSwapchain(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Surface,
                                 {framebufferSize.x, framebufferSize.y, framebufferSize.width, framebufferSize.height}, m_Device->GetPresentQueueFamilyIndex());

    m_VertexBuffer = new FVkBuffer();
    m_IndexBuffer = new FVkBuffer();
    m_PointLightsBuffer = new FVkBuffer();

    m_MultisampledRenderTarget = new FVkMultisampler();
    m_MultisampledRenderTarget->Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_SAMPLE_COUNT_1_BIT,
                                     m_Swapchain->GetSwapchainExtent().width, m_Swapchain->GetSwapchainExtent().height, m_Swapchain->GetImageFormat());

    m_FramesInFlight = m_Swapchain->GetSwapchainImageCount();
    m_FramesInFlight = 3;

    m_SceneNodeTransformsLayout =
        FVkDescriptorSetLayout::Builder(m_Device->GetLogicalDevice()).add(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1).build(0);
    m_CameraUboLayout =
        FVkDescriptorSetLayout::Builder(m_Device->GetLogicalDevice()).add(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1).build(0);

    VkExtent2D swapchainExtent = m_Swapchain->GetSwapchainExtent();

    m_ImmediateCommandPool = new FVkCommandPool();
    m_ImmediateCommandPool->Init(m_Device->GetLogicalDevice(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, m_Device->GetGraphicsQueueFamilyIndex());

    createFallbackTexture(fallback);

    m_Frames.resize(m_FramesInFlight);
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (size_t i = 0; i < m_FramesInFlight; i++)
    {
        Frame& frame = m_Frames[i];

        frame.scene.m_SceneNodeTransformsStorageBuffer.Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(),
                                                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                            NODE_TRANSFORMS_MAX_CUP * sizeof(Fleur::Mat4) + sizeof(Fleur::Mat4), sizeof(Fleur::Mat4));

        frame.scene.m_CameraBuffer.Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(),
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(SFLGeometryUBO), sizeof(Fleur::Mat4));

        std::vector<vk::abstraction::DescriptorAllocator::PoolSizeRatio> frame_sizes = {
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_FramesInFlight},          // ssbo
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_FramesInFlight},          // camera data
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_FramesInFlight},  // shadowMap
        };
        frame.frameDescriptors.init(m_Device->GetLogicalDevice(), 1000, frame_sizes);

        m_ShadowMapLayout = FVkDescriptorSetLayout::Builder(m_Device->GetLogicalDevice())
                                .add(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
                                .build(0);

        frame.scene.m_SceneNodeTransformsDescriptor =
            frame.frameDescriptors.allocate(m_Device->GetLogicalDevice(), m_SceneNodeTransformsLayout->GetDescriptorSetLayout(), 1);
        frame.scene.m_CameraDescriptor = frame.frameDescriptors.allocate(m_Device->GetLogicalDevice(), m_CameraUboLayout->GetDescriptorSetLayout(), 1);
        frame.scene.m_ShadowMapDescriptorSet = frame.frameDescriptors.allocate(m_Device->GetLogicalDevice(), m_ShadowMapLayout->GetDescriptorSetLayout(), 1);

        vk::abstraction::DescriptorWriter ssboWriter{};
        ssboWriter.write_buffer(0, frame.scene.m_SceneNodeTransformsStorageBuffer.GetBuffer(), sizeof(Fleur::Graphics::SFLSSBODescriptorBuffer), 0,
                                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        ssboWriter.update_set(m_Device->GetLogicalDevice(), frame.scene.m_SceneNodeTransformsDescriptor);

        vk::abstraction::DescriptorWriter cameraUboWriter{};
        cameraUboWriter.write_buffer(0, frame.scene.m_CameraBuffer.GetBuffer(), sizeof(Fleur::Graphics::SFLGeometryUBO), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        cameraUboWriter.update_set(m_Device->GetLogicalDevice(), frame.scene.m_CameraDescriptor);

        m_ImageSampler = createTextureSampler();

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

    m_ShadowMapRenderTargets.resize(m_FramesInFlight);
    for (auto& shadowMap : m_ShadowMapRenderTargets)
        shadowMap.Create(m_Device, m_ImmediateCommandPool, swapchainExtent, VK_SAMPLE_COUNT_1_BIT, true);
    m_ShadowMapSampler = createShadowMapSampler();
    updateShadowMapDescriptorSets();


    m_DepthRenderTarget.Create(m_Device, m_ImmediateCommandPool, swapchainExtent, m_MultisampledRenderTarget->GetSamplesCount());

    m_VertexBuffer->Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                         1024u * 1024ul * 512ul, sizeof(Fleur::Graphics::SVertexData));

    m_IndexBuffer->Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                        1024u * 1024ul * 256ul, sizeof(uint32_t));

    m_PointLightsLayout = FVkDescriptorSetLayout::Builder(m_Device->GetLogicalDevice())
                              .add(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
                              .build(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT);
    m_PointLightsBuffer->Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(),
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, POINT_LIGHTS_MAX_CUP * sizeof(SFLPointLight),
                              sizeof(SFLPointLight));

    createTextureDescriptorSetPass();

    updateTextureDescriptorSet(m_TextureDescriptorSet, m_FallbackTextureIdx, m_TextureMap[m_FallbackTextureIdx].GetImageView(), m_ImageSampler);
    updateTextureDescriptorSet(m_TextureDescriptorSet, kDebugShadowMapTextureSlot, m_ShadowMapRenderTargets.front().GetImageView(), m_ShadowMapSampler);

    m_DescriptorSetImageViewsToUpload.resize(m_FramesInFlight);

    {
        FVkSingleTimeCommandBuffer frameCmd = FVkSingleTimeCommandBuffer(m_Device->GetLogicalDevice(), m_ImmediateCommandPool->GetCommandPool());

        frameCmd.TransitionImageLayout(m_MultisampledRenderTarget->GetTexture()->GetImage(), m_Swapchain->GetImageFormat(), VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, 1);

        frameCmd.Submit(m_Device->GetGraphicsQueue());
    }

    m_OverlayPass = new FVkOverlayPass();
    m_OverlayPass->Create(m_Device, m_Swapchain, m_TextureDescriptorSetLayout->GetDescriptorSetLayout(), m_TextureDescriptorSet, kDebugShadowMapTextureSlot,
                          m_MultisampledRenderTarget->GetSamplesCount(), FVkDepthTarget::FindDepthFormat(m_Device->GetPhysicalDevice()), m_FramesInFlight);

    m_DebugDraw = new FVkDebugDraw();

    // Create the random offset texture and the descriptor set that owns it.
    m_ShadowMapOffsetTexture.Create(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_ImmediateCommandPool->GetCommandPool(),
                                    m_Device->GetGraphicsQueue(), m_Swapchain->GetSwapchainExtent(), 3);
}

vk::backend::impl::~impl()
{
    vkDeviceWaitIdle(m_Device->GetLogicalDevice());

    m_ShadowMapOffsetTexture.Destroy();

    delete m_OverlayPass;
    delete m_Skybox;
    delete m_Floor;
    delete m_DebugDraw;

    uint32_t framebuffersCount = m_FramesInFlight;

    // 1. Synchronization objects
    for (size_t i = 0; i < framebuffersCount; i++)
    {
        Frame& frame = m_Frames[i];
        vkDestroySemaphore(m_Device->GetLogicalDevice(), frame.m_ImagesAvailable, nullptr);
        vkDestroyFence(m_Device->GetLogicalDevice(), frame.m_InFlightFences, nullptr);
    }
    destroyRenderFinishedSemaphores();

    // 2. CommandBuffer & CommandPool
    delete m_ImmediateCommandPool;

    // 3. DescriptorSet & DescriptorPool & Descriptor set layout
    vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), m_DescriptorPool, nullptr);
    delete m_TextureDescriptorSetLayout;
    delete m_CameraUboLayout;
    delete m_SceneNodeTransformsLayout;
    delete m_PointLightsLayout;
    delete m_ShadowMapLayout;

    // 4. Pipeline
    delete m_GeometryPipeline;
    delete m_TransparentPipeline;
    delete m_ShadowPipeline;

    // 5. Swapchain & Framebuffers & swapchain image views

    // 7. All ImageViews
    delete m_MultisampledRenderTarget;
    delete m_FallbackCubemapTexture;
    m_TextureMap.clear();
    m_Swapchain->ReleaseSwapchainImageViews();

    // 8. Buffers
    delete m_VertexBuffer;
    delete m_IndexBuffer;
    delete m_PointLightsBuffer;
    // 9. Samplers
    vkDestroySampler(m_Device->GetLogicalDevice(), m_ImageSampler, nullptr);
    vkDestroySampler(m_Device->GetLogicalDevice(), m_ShadowMapSampler, nullptr);

    // 10. Swapchain
    delete m_Swapchain;

    // Shadow-map images own Vulkan resources and must be released before the device.
    m_ShadowMapRenderTargets.clear();

    // 15. VMA
    freeVma();

    // 11. Surface
    vkDestroySurfaceKHR(m_VulkanInstance, m_Surface, nullptr);

    // 12. LogicalDevice
    delete m_Device;

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


FVkPipeline* vk::backend::impl::createGraphicsPipeline(const char* shaderKey, Fleur::Graphics::SFLShaderInfo pVertexInfo,
                                                       Fleur::Graphics::SFLShaderInfo pFragmentInfo, const vk::GetPipelineInfo& pipelineInfo,
                                                       const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
{
    vk::ShaderCreateInfo shaderCreateInfo{.pVertexData = pVertexInfo.shaderCode,
                                          .vertexSize = pVertexInfo.sizeBytes,
                                          .pFragmentData = pFragmentInfo.shaderCode,
                                          .fragmentSize = pFragmentInfo.sizeBytes};

    auto& shader = m_ShaderMap.emplace(shaderKey, vk::FVkShader()).first->second;
    if (!shader.isInitialized())
        shader.Init(m_Device->GetLogicalDevice(), shaderCreateInfo);

    return shader.GetPipeline(pipelineInfo, descriptorSetLayouts);
}

FVkPipeline* vk::backend::impl::createGeometryPipeline(Fleur::Graphics::SFLShaderInfo pVertexInfo, Fleur::Graphics::SFLShaderInfo pFragmentInfo,
                                                       VkSampleCountFlagBits samplesCount, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
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

    return createGraphicsPipeline("opaque", pVertexInfo, pFragmentInfo, pipelineInfo, descriptorSetLayouts);
}

FVkPipeline* vk::backend::impl::createTransparentPipeline(Fleur::Graphics::SFLShaderInfo pVertexInfo, Fleur::Graphics::SFLShaderInfo pFragmentInfo,
                                                          VkSampleCountFlagBits samplesCount, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
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

    return createGraphicsPipeline("opaque", pVertexInfo, pFragmentInfo, pipelineInfo, descriptorSetLayouts);
}

FVkPipeline* vk::backend::impl::createShadowPipeline(Fleur::Graphics::SFLShaderInfo pVertexInfo, Fleur::Graphics::SFLShaderInfo pFragmentInfo,
                                                     VkSampleCountFlagBits samplesCount, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
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

    return createGraphicsPipeline("shadow", pVertexInfo, pFragmentInfo, pipelineInfo, descriptorSetLayouts);
}

VkShaderModule vk::backend::impl::createShaderModule(Fleur::Graphics::SFLShaderInfo* pShaderInfo)
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
    poolSizes[0].descriptorCount = MAX_TEXTURES + 1;

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
                                        .maxSets = 3,
                                        .poolSizeCount = poolSizes.size(),
                                        .pPoolSizes = poolSizes.data()};

    VK_CHECK(vkCreateDescriptorPool(m_Device->GetLogicalDevice(), &poolInfo, nullptr, &m_DescriptorPool));
}
void vk::backend::impl::createTextureDescriptorSets()
{
    // ---------- textures ----------
    auto textureDsl = m_TextureDescriptorSetLayout->GetDescriptorSetLayout();

    VkDescriptorSetAllocateInfo texturesDescriptorSetAllocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = m_DescriptorPool, .descriptorSetCount = 1, .pSetLayouts = &textureDsl};

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

    // ---------- PointLights ----------
    auto pointLightsDsl = m_PointLightsLayout->GetDescriptorSetLayout();

    VkDescriptorSetAllocateInfo pointLightsDescriptorSetAllocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = m_DescriptorPool, .descriptorSetCount = 1, .pSetLayouts = &pointLightsDsl};

    VK_CHECK(vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &pointLightsDescriptorSetAllocInfo, &m_PointLightDescriptorSet));

    VkDescriptorBufferInfo pointLightBufferInfo{};
    pointLightBufferInfo.buffer = m_PointLightsBuffer->GetBuffer();
    pointLightBufferInfo.offset = 0;
    pointLightBufferInfo.range = POINT_LIGHTS_MAX_CUP * sizeof(Fleur::Graphics::SFLPointLight);

    VkWriteDescriptorSet pointLightDescriptorWrites{};
    pointLightDescriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    pointLightDescriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pointLightDescriptorWrites.dstSet = m_PointLightDescriptorSet;
    pointLightDescriptorWrites.dstBinding = 0;
    pointLightDescriptorWrites.dstArrayElement = 0;
    pointLightDescriptorWrites.descriptorCount = 1;
    pointLightDescriptorWrites.pBufferInfo = &pointLightBufferInfo;

    vkUpdateDescriptorSets(m_Device->GetLogicalDevice(), 1, &pointLightDescriptorWrites, 0, nullptr);
}

void vk::backend::impl::initializeVma()
{
    VmaAllocatorCreateInfo allocCreateInfo{};
    allocCreateInfo.instance = m_VulkanInstance;
    allocCreateInfo.physicalDevice = m_Device->GetPhysicalDevice();
    allocCreateInfo.device = m_Device->GetLogicalDevice();
    allocCreateInfo.vulkanApiVersion = VULKAN_VERSION;

    VK_CHECK(vmaCreateAllocator(&allocCreateInfo, &m_Allocator));
}
void vk::backend::impl::freeVma()
{
    vmaDestroyAllocator(m_Allocator);
}

void vk::backend::impl::uploadTextures(Fleur::Graphics::SFLImageViewInfo* pInfo)
{
    for (size_t i = 0; i < pInfo->count; i++)
    {
        auto& imageView = pInfo->pData[i];
        if (m_TextureMap.contains(imageView.ID))
            continue;

        VkFormat format = m_Device->GetTextureFormat(imageView.channels);
        VkImageAspectFlagBits aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        uint32_t layerSize = imageView.w * imageView.h * imageView.channels;
        uint32_t imageSize = layerSize * imageView.layerCount;
        uint32_t mimMapLevel = 1;
        if (imageView.layerCount == 1)
            mimMapLevel = CalculateMimMapLevel(imageView.w, imageView.h);
        auto& gpuTexture = m_TextureMap.emplace(imageView.ID, FVkTexture()).first->second;

        createTexture(imageView, gpuTexture, format, aspect, mimMapLevel, imageView.layerCount);

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
    for (VkSemaphore& semaphore : m_RenderFinished)
        VK_CHECK(vkCreateSemaphore(m_Device->GetLogicalDevice(), &semaphoreInfo, nullptr, &semaphore));
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

    uint32_t mimMapCount = CalculateMimMapLevel(view.w, view.h);
    createTexture(view, *fallbackTexture, format, aspect, mimMapCount, 1);


    // ---------- cubemap texture placeholder ----------
    m_FallbackCubemapTexture = new FVkTexture();

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

    char* buffer = new char[imageSize];
    for (size_t i = 0; i < CUBEMAP_LAYERS_COUNT; i++)
    {
        memcpy(buffer + (layerSize * i), view.pData, layerSize);
    }

    FVkBuffer stagingBuffer{};
    stagingBuffer.Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, imageSize, layerSize);
    stagingBuffer.MemCopy(buffer, imageSize);

    VkImage cubemapImage = m_FallbackCubemapTexture->CreateImage(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), imageInfo,
                                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cubemapAspect);

    {
        FVkSingleTimeCommandBuffer frameCmd = FVkSingleTimeCommandBuffer(m_Device->GetLogicalDevice(), m_ImmediateCommandPool->GetCommandPool());
        frameCmd.TransitionImageLayout(cubemapImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cubemapAspect, cubemapMimMapCount,
                                       CUBEMAP_LAYERS_COUNT);
        frameCmd.CopyBufferToImage(stagingBuffer.GetBuffer(), cubemapImage, VkExtent2D{view.w, view.h}, layerSize, CUBEMAP_LAYERS_COUNT);
        frameCmd.TransitionImageLayout(cubemapImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cubemapAspect,
                                       cubemapMimMapCount, CUBEMAP_LAYERS_COUNT);
        frameCmd.Submit(m_Device->GetGraphicsQueue());
    }
    m_FallbackCubemapTexture->CreateImaveView();

    delete buffer;
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

    VkImage vkImage = texture.CreateImage(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, aspect);

    VkDeviceSize layerSize = view.w * view.h * channels;
    VkDeviceSize imageSize = layerSize * view.layerCount;


    FVkBuffer stagingBuffer{};
    stagingBuffer.Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, imageSize, layerSize);

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

    texture.CreateImaveView();
}

void vk::backend::impl::startResize()
{
    m_WindowResizeIsInProgress = true;
    std::cout << "\nStartResize\n";
}
void vk::backend::impl::endResize(Fleur::SRect& rect)
{
    m_WindowResizeIsInProgress = false;
    vkDeviceWaitIdle(m_Device->GetLogicalDevice());
    m_Swapchain->OnWindowResized(rect);
    m_DepthRenderTarget.Recreate({rect.width, rect.height}, m_MultisampledRenderTarget->GetSamplesCount());
    for (auto& shadowMap : m_ShadowMapRenderTargets)
        shadowMap.Recreate({rect.width, rect.height}, VK_SAMPLE_COUNT_1_BIT, true);
    updateShadowMapDescriptorSets();
    updateTextureDescriptorSet(m_TextureDescriptorSet, kDebugShadowMapTextureSlot, m_ShadowMapRenderTargets.front().GetImageView(), m_ShadowMapSampler);
    std::cout << "\nEndResize\n";
}

void vk::backend::impl::createSkybox(AssetID id, SFLShaderStages shaderStages)
{
    if (m_Skybox)
        return;

    assert(shaderStages.vertex.shaderCode);
    assert(shaderStages.fragment.shaderCode);

    vk::ShaderCreateInfo shaderCreateInfo{.pVertexData = shaderStages.vertex.shaderCode,
                                          .vertexSize = shaderStages.vertex.sizeBytes,
                                          .pFragmentData = shaderStages.fragment.shaderCode,
                                          .fragmentSize = shaderStages.fragment.sizeBytes};

    auto& skyboxShader = m_ShaderMap.emplace("skybox", vk::FVkShader()).first->second;
    skyboxShader.Init(m_Device->GetLogicalDevice(), shaderCreateInfo);

    m_Skybox = new FVkSkybox();
    m_Skybox->Create(m_Device, m_Swapchain, m_FallbackCubemapTexture->GetImageView(), &skyboxShader, m_MultisampledRenderTarget->GetSamplesCount(),
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

    assert(shaderStages.vertex.shaderCode && shaderStages.fragment.shaderCode);

    vk::ShaderCreateInfo shaderCreateInfo{.pVertexData = shaderStages.vertex.shaderCode,
                                          .vertexSize = shaderStages.vertex.sizeBytes,
                                          .pFragmentData = shaderStages.fragment.shaderCode,
                                          .fragmentSize = shaderStages.fragment.sizeBytes};
    auto& floorShader = m_ShaderMap.emplace("floor", vk::FVkShader()).first->second;
    floorShader.Init(m_Device->GetLogicalDevice(), shaderCreateInfo);

    m_FloorTextureIdx = texture;
    m_Floor = new FVkFloor();
    uint32_t idx = m_FloorTextureIdx;
    if (m_TextureMap.find(texture) == m_TextureMap.end())
        idx = m_FallbackTextureIdx;

    m_Floor->Create(m_Device, m_Swapchain, &floorShader, m_CameraUboLayout->GetDescriptorSetLayout(), m_TextureMap[idx].GetImageView(), height,
                    m_MultisampledRenderTarget->GetSamplesCount(), FVkDepthTarget::FindDepthFormat(m_Device->GetPhysicalDevice()));
}

void vk::backend::impl::setFloor(AssetID texture, float height)
{
    if (m_Floor)
        m_Floor->SetFloor(m_TextureMap[texture].GetImageView(), height);
}

void vk::backend::impl::createPass(EFLPassKind kind, SFLShaderStages shaderStages)
{
    if (kind == EFLPassKind::Geometry)
    {
        // TODO if pipeline already exists, need to release it
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {
            m_CameraUboLayout->GetDescriptorSetLayout(),           m_TextureDescriptorSetLayout->GetDescriptorSetLayout(),
            m_SceneNodeTransformsLayout->GetDescriptorSetLayout(), m_PointLightsLayout->GetDescriptorSetLayout(),
            m_ShadowMapLayout->GetDescriptorSetLayout(),            m_ShadowMapOffsetTexture.GetDescriptorSetLayout(),
        };
        m_GeometryPipeline =
            createGeometryPipeline(shaderStages.vertex, shaderStages.fragment, m_MultisampledRenderTarget->GetSamplesCount(), descriptorSetLayouts);
        m_TransparentPipeline =
            createTransparentPipeline(shaderStages.vertex, shaderStages.fragment, m_MultisampledRenderTarget->GetSamplesCount(), descriptorSetLayouts);
    }
    else if (kind == EFLPassKind::Shadow)
    {
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {
            m_SceneNodeTransformsLayout->GetDescriptorSetLayout(),
        };
        m_ShadowPipeline = createShadowPipeline(shaderStages.vertex, shaderStages.fragment, VK_SAMPLE_COUNT_1_BIT, descriptorSetLayouts);
    }
}

void vk::backend::impl::updatePointLight(const SFLPointLight* light, uint32_t lightCount)
{
    m_PointLights.clear();
    m_PointLights.reserve(lightCount);
    m_PointLights.insert(m_PointLights.begin(), lightCount, *light);

    assert(m_PointLights.size() <= POINT_LIGHTS_MAX_CUP);

    m_PointLightsBuffer->UploadDataToBuffer(light, lightCount);
}

void vk::backend::impl::updateShadowMapDescriptorSets()
{
    for (size_t i = 0; i < m_Frames.size(); ++i)
    {
        vk::abstraction::DescriptorWriter shadowMapWriter{};
        shadowMapWriter.write_image(0, m_ShadowMapRenderTargets[i].GetImageView(), m_ShadowMapSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        auto& frame = m_Frames[i];
        shadowMapWriter.update_set(m_Device->GetLogicalDevice(), frame.scene.m_ShadowMapDescriptorSet);
    }
}

void vk::backend::impl::BeginRendering(VkCommandBuffer cmd, const FBeginRenderingDesc& desc)
{
    VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
    VkRenderingAttachmentInfoKHR depthAttachmentInfo{};

    uint32_t colorAttachmentCount = 0;
    const VkRenderingAttachmentInfoKHR* pColorAttachments = nullptr;
    const VkRenderingAttachmentInfoKHR* pDepthAttachment = nullptr;

    if (desc.colorAttachment)
    {
        colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorAttachmentInfo.pNext = nullptr;
        colorAttachmentInfo.imageView = desc.colorAttachment->imageView;
        colorAttachmentInfo.imageLayout = desc.colorAttachment->imageLayout;
        colorAttachmentInfo.resolveMode = desc.colorAttachment->resolveMode;
        colorAttachmentInfo.resolveImageView = desc.colorAttachment->resolveImageView;
        colorAttachmentInfo.resolveImageLayout = desc.colorAttachment->resolveImageLayout;
        colorAttachmentInfo.loadOp = desc.colorAttachment->loadOp;
        colorAttachmentInfo.storeOp = desc.colorAttachment->storeOp;
        colorAttachmentInfo.clearValue = desc.colorAttachment->clearValue;

        colorAttachmentCount = 1;
        pColorAttachments = &colorAttachmentInfo;
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

void vk::backend::impl::ExecuteShadowPass()
{
    auto& cmd = GetCurrentFrame().m_CommandBuffers;
    auto& shadowMap = m_ShadowMapRenderTargets[m_CurrentFrame];

    transitionImageLayout(*cmd.GetCommandBuffer(), shadowMap.GetImage(), FVkDepthTarget::FindDepthFormat(m_Device->GetPhysicalDevice()),
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    VkRect2D renderArea{
        .offset = {0, 0},
        .extent = {.width = m_Swapchain->GetSwapchainExtent().width, .height = m_Swapchain->GetSwapchainExtent().height},
    };

    FRenderingDepthAttachmentDesc depthDesc{};
    depthDesc.imageView = shadowMap.GetImageView();
    depthDesc.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthDesc.clearValue.depthStencil = {.depth = 1.0f, .stencil = 0};

    FBeginRenderingDesc renderingDesc{};
    renderingDesc.renderArea = renderArea;

    // No color attachment for shadow map.
    renderingDesc.colorAttachment = nullptr;
    renderingDesc.depthAttachment = &depthDesc;

    renderingDesc.layerCount = 1;
    renderingDesc.viewMask = 0;

    BeginRendering(*cmd.GetCommandBuffer(), renderingDesc);

    cmd.BindVertexBuffer(&m_VertexBuffer->GetBuffer());
    cmd.BindIndexBuffer(&m_IndexBuffer->GetBuffer(), VK_INDEX_TYPE_UINT32);

    // We need to bind shadow pass pipeline
    cmd.BindPipeline(m_ShadowPipeline->GetPipeline());

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
}


void vk::backend::impl::ExecuteMainPass()
{
    auto& cmd = GetCurrentFrame().m_CommandBuffers;

    transitionImageLayout(*cmd.GetCommandBuffer(), m_Swapchain->GetSwapchainImage(m_ImageIndex), m_Swapchain->GetImageFormat(), VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1);

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
        // m_Floor->Record(*cmd.GetCommandBuffer(), GetCurrentFrame().scene.m_CameraDescriptor, m_Swapchain->GetSwapchainExtent(), m_FrameData.camera);
    }

    cmd.BindVertexBuffer(&m_VertexBuffer->GetBuffer());
    cmd.BindIndexBuffer(&m_IndexBuffer->GetBuffer(), VK_INDEX_TYPE_UINT32);


    cmd.BindPipeline(m_GeometryPipeline->GetPipeline());

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
}

void vk::backend::impl::createTextureDescriptorSetPass()
{
    m_TextureDescriptorSetLayout = FVkDescriptorSetLayout::Builder(m_Device->GetLogicalDevice())
                                       .add(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, MAX_TEXTURES)
                                       .build(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT);

    createTextureDescriptorPool();
    createTextureDescriptorSets();
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
        createRenderFinishedSemaphores();
    }
    m_FrameData = frameData;

    Frame& frame = GetCurrentFrame();

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

    if (m_ImageIndex >= m_RenderFinished.size())
    {
        DBG_PRINTM("Acquired swap chain image index is outside render-finished semaphore array")
        return false;
    }

    if (!m_GeometryPipeline)
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

    Fleur::Graphics::SFLGeometryUBO ubo{m_FrameData.camera.view, m_FrameData.camera.proj};
    GetCurrentFrame().scene.m_CameraBuffer.MemCopy(&ubo, sizeof(ubo));

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
    Frame& frame = GetCurrentFrame();
    auto& cmd = frame.m_CommandBuffers;

    cmd.CmdBeginDebugLabel(vk::myVkCmdBeginDebugUtilsLabelEXT, "Shadow Pass");
    ExecuteShadowPass();
    cmd.BindDescriptorSets(m_ShadowPipeline->GetPipelineLayout(), &frame.scene.m_SceneNodeTransformsDescriptor, 1);

    const auto& shadowFrustum = m_ShadowMapFrustumSettings;
    Fleur::Vec3 lightPos = Fleur::Vec3(m_FrameData.directionalLight.pos);

    // Directional light has no real position.
    // This is only a virtual camera position for shadow rendering.
    float halfSize = shadowFrustum.halfSize;
    const float lightDistance = Fleur::Math::length(lightPos);
    const float shadowNear = lightDistance * shadowFrustum.nearDistanceFactor;
    const float shadowFar = lightDistance + shadowFrustum.farExtension;

    Fleur::Vec3 up(0.0f, 1.0f, 0.0f);
    Fleur::Vec3 lightDirection = Fleur::Math::normalize(Fleur::Vec3(m_FrameData.directionalLight.dirIntens));

    if (Fleur::Math::abs(Fleur::Math::dot(lightDirection, up)) > 0.99f)
        up = Fleur::Vec3(1.0f, 0.0f, 0.0f);

    Fleur::Vec3 shadowEnd = lightPos + lightDirection * shadowFar;
    Fleur::Mat4 lightView = Fleur::Math::lookAt(lightPos, shadowEnd, up);

    // Fleur::Mat4 lightProjection = Fleur::Math::orthoRH_ZO(-halfSize, halfSize, -halfSize, halfSize, shadowNear, shadowFar);
    Fleur::Mat4 lightProjection = Fleur::Math::orthoRH_ZO(-halfSize, halfSize, -halfSize, halfSize, shadowNear, shadowFar);
    lightProjection[1][1] *= -1;

    Fleur::Mat4 lightSpaceMatrix = lightProjection * lightView;

    if (shadowFrustum.drawDebugFrustum)
        m_DebugDraw->Frustum(Fleur::Math::inverse(lightSpaceMatrix), Fleur::Vec3(0.0f, 1.0f, 1.0f));

    for (const auto& drawItem : m_OpaqueDrawItems)
    {
        const auto& primitive = m_Primitives[drawItem.primitiveIdx];
        struct ShadowPuchConstant
        {
            Fleur::Mat4 lightSpaceMatrix;
            uint32_t modelIdx;
            uint32_t nodeIdx;
        } pc;
        pc.lightSpaceMatrix = lightSpaceMatrix;
        pc.modelIdx = drawItem.modelTransformIdx;
        pc.nodeIdx = drawItem.nodeTransformsStartIdx;
        cmd.PushConstant(m_ShadowPipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, pc);
        cmd.DrawIndexed(primitive.indexCount, primitive.indexOffset, primitive.vertexOffset, drawItem.instanceCount, 0);
    }
    cmd.EndRendering();
    cmd.CmdEndDebugLabel(vk::myVkCmdEndDebugUtilsLabelEXT);

    std::array<VkDescriptorSet, 6> dst{frame.scene.m_CameraDescriptor, m_TextureDescriptorSet, frame.scene.m_SceneNodeTransformsDescriptor,
                                       m_PointLightDescriptorSet, frame.scene.m_ShadowMapDescriptorSet, m_ShadowMapOffsetTexture.GetDescriptorSet()};

    auto& shadowMap = m_ShadowMapRenderTargets[m_CurrentFrame];
    transitionImageLayout(*cmd.GetCommandBuffer(), shadowMap.GetImage(), FVkDepthTarget::FindDepthFormat(m_Device->GetPhysicalDevice()),
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    cmd.CmdBeginDebugLabel(vk::myVkCmdBeginDebugUtilsLabelEXT, "Main Pass");
    ExecuteMainPass();

    updateTextureDescriptorSet(m_TextureDescriptorSet, kDebugShadowMapTextureSlot, shadowMap.GetImageView(), m_ShadowMapSampler);

    cmd.BindDescriptorSets(m_GeometryPipeline->GetPipelineLayout(), dst.data(), dst.size());

    for (const auto& drawItem : m_OpaqueDrawItems)
    {
        const auto& primitive = m_Primitives[drawItem.primitiveIdx];
        SFLPushConstant pushConstant = MakePush(primitive);
        pushConstant.lightSpaceMatrix = lightSpaceMatrix;
        pushConstant.directionalLightColor = m_FrameData.directionalLight.color;
        pushConstant.directionalLightDirectionIntensity = m_FrameData.directionalLight.dirIntens;
        pushConstant.indices.x = drawItem.nodeTransformsStartIdx;
        pushConstant.indices.y = drawItem.modelTransformIdx;
        pushConstant.indices.w = m_PointLights.size();
        pushConstant.cameraPos = Fleur::Math::inverse(m_FrameData.camera.view)[3];
        cmd.PushConstant(m_GeometryPipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstant);
        cmd.DrawIndexed(primitive.indexCount, primitive.indexOffset, primitive.vertexOffset, drawItem.instanceCount, 0);
    }
    //

    cmd.BindVertexBuffer(&m_VertexBuffer->GetBuffer());
    cmd.BindIndexBuffer(&m_IndexBuffer->GetBuffer(), VK_INDEX_TYPE_UINT32);
    cmd.BindPipeline(m_TransparentPipeline->GetPipeline());

    for (const auto& drawItem : m_TransparentDrawItems)
    {
        const auto& primitive = m_Primitives[drawItem.primitiveIdx];
        SFLPushConstant pushConstant = MakePush(primitive);
        pushConstant.indices.x = drawItem.nodeTransformsStartIdx;
        pushConstant.indices.y = drawItem.modelTransformIdx;
        pushConstant.indices.w = m_PointLights.size();
        cmd.PushConstant(m_TransparentPipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstant);
        cmd.DrawIndexed(primitive.indexCount, primitive.indexOffset, primitive.vertexOffset, drawItem.instanceCount, 0);
    }

    m_OpaqueDrawItems.clear();
    m_TransparentDrawItems.clear();

    cmd.CmdEndDebugLabel(vk::myVkCmdEndDebugUtilsLabelEXT);

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
    cmd.End();

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

    vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);

    m_CurrentFrame = (m_CurrentFrame + 1) % m_FramesInFlight;
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
    Fleur::Mat4* matrices = new Fleur::Mat4[matricesCount];

    matrices[0] = modelTransform;
    matrices[1] = Fleur::Mat4(Fleur::Math::transpose(Fleur::Math::inverse(Fleur::Mat3(modelTransform))));

    memcpy(&matrices[2], &m_InstanceNodeTransforms[batch.globalNodeStartIdx], sizeof(Fleur::Mat4) * batch.nodeTransformCount);

    uint32_t ssboCurrentIdx = frame.scene.m_SceneNodeTransformsStorageBuffer.CurrentSize() / frame.scene.m_SceneNodeTransformsStorageBuffer.StrideBytes();
    frame.scene.m_SceneNodeTransformsStorageBuffer.UploadDataToBuffer(matrices, matricesCount);
    delete[] matrices;

    for (size_t i = 0; i < batch.instancesCount; i++)
    {
        const auto& instance = srcInstance[i];
        uint32_t localNodeOffset = instance.globalNodeTransformStartIdx - batch.globalNodeStartIdx;
        uint32_t ssboNodeOffset = ssboCurrentIdx + localNodeOffset + 2;

        for (size_t j = 0; j < instance.primitiveCount; j++)
        {
            const auto& primitive = m_Primitives[instance.globalPrimitiveStartIdx + j];
            const uint32_t globalPrimitiveIdx = instance.globalPrimitiveStartIdx + j;

            std::vector<FLFrameDrawItem>* dstVector{nullptr};
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
