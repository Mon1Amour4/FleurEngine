
// This entire .cpp file was so big so it was pain in the ass to navigate throughout
// I've hidden vulkanBackendImpl declaration into .hpp file
#if defined(FLEUR_PLATFORM_WIN)
#define NOMINMAX
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(FLEUR_PLATFORM_LINUX)
#define VK_USE_PLATFORM_WAYLAND_KHR
#else
#error
#endif

#include "PrivateVulkanImpl.hpp"


// ---------- backend ----------
vk::backend::backend(bool enableValidation, Fleur::Graphics::SFLFrame& pFrame, void* pNativeHandle, Fleur::SRect& framebufferSize,
                     Fleur::Graphics::SFLImageView& fallback)
    : pImpl(new vk::backend::impl(enableValidation, pFrame, pNativeHandle, framebufferSize, fallback))
{
}
vk::backend::~backend()
{
    delete pImpl;
}
void vk::backend::AddToDrawList(Fleur::Graphics::SFLModelView* pModelView)
{
    pImpl->addToDrawList(pModelView);
}
void vk::backend::Update(Fleur::Graphics::SFLCameraData& cameraData)
{
    pImpl->update(cameraData);
}
void vk::backend::SubmitImageViews(Fleur::Graphics::SFLImageViewInfo* pInfo)
{
    pImpl->submitImageViews(pInfo);
}
void vk::backend::CreateSkybox(AssetID id, SFLShaderInfo* pVertexShaderInfo, SFLShaderInfo* pFragmentShaderInfo)
{
    pImpl->createSkybox(id, pVertexShaderInfo, pFragmentShaderInfo);
}
void vk::backend::SetSkybox(AssetID id)
{
    pImpl->setSkybox(id);
}
void vk::backend::StartResize()
{
    pImpl->startResize();
}
void vk::backend::EndResize(Fleur::SRect& rect)
{
    pImpl->endResize(rect);
}


// ---------- impl ----------
// clang-format off
vk::backend::impl::impl(bool enableValidation, 
                        Fleur::Graphics::SFLFrame& pFrame, 
                        void* pNativeHandle, Fleur::SRect& framebufferSize,
                        Fleur::Graphics::SFLImageView& fallback)
    // clang-format on
    : m_WindowResizeIsInProgress(false)
    , m_StaticGeometryTexturesDsl(nullptr)
    , m_StaticGeometryUboDsl(nullptr)
    , m_FrameContext(nullptr)
    , m_Skybox(nullptr)
    , m_FallbackCubemapTexture(nullptr)
    , m_DepthRenderTarget(nullptr)
{
    assert(pFrame.pPass->pVertexShaderInfo->shaderCode);
    assert(pFrame.pPass->pFragmentShaderInfo->shaderCode);

    std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};
    std::vector<const char*> instanceExtensions{"VK_EXT_debug_utils", "VK_KHR_surface"};
#if defined(FLEUR_PLATFORM_WIN)
    instanceExtensions.push_back("VK_KHR_win32_surface");
#elif defined(FLEUR_PLATFORM_LINUX)
    instanceExtensions.push_back("VK_KHR_wayland_surface");
#else
#error
#endif
    m_VulkanInstance = createInstance(enableValidation, instanceExtensions, validationLayers);
    setupDebugMessenger();

    m_Swapchain = new FVkSwapchain();
    m_Surface = createSurface(m_VulkanInstance, pNativeHandle);

    SDeviceInfo deviceInfo{};
    deviceInfo.presentationSupport = true;
    deviceInfo.neededQueueFamilyFlags = VK_QUEUE_GRAPHICS_BIT;
    deviceInfo.surface = m_Surface;
    deviceInfo.requiredDeviceExtensions = deviceExtensions;

    m_Device = FVkDevice::CreateSuitableDevice(m_VulkanInstance, deviceInfo);
    m_Device->CreateLogicalDevice(deviceExtensions);

    initializeVma();
    m_Swapchain->CreateSwapchain(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Surface,
                                 {framebufferSize.x, framebufferSize.y, framebufferSize.width, framebufferSize.height}, m_Device->GetPresentQueueFamilyIndex());

    m_VertexBuffer = new FVkBuffer();
    m_IndexBuffer = new FVkBuffer();

    m_GeometryVertexInput = new SFLVertexInput();
    m_GeometryVertexInput->RegisterAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Fleur::Graphics::SVertexData, Position));
    m_GeometryVertexInput->RegisterAttribute(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Fleur::Graphics::SVertexData, TexCoord));
    m_GeometryVertexInput->RegisterAttribute(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Fleur::Graphics::SVertexData, Normal));

    m_MultisampledRenderTarget = new FVkMultisampler();
    m_MultisampledRenderTarget->Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_SAMPLE_COUNT_1_BIT,
                                     m_Swapchain->GetSwapchainExtent().width, m_Swapchain->GetSwapchainExtent().height, m_Swapchain->GetImageFormat());

    uint32_t swapChainImageCount = m_Swapchain->GetSwapchainImageCount();


    m_FrameContext = new FrameContext(swapChainImageCount);
    for (size_t i = 0; i < swapChainImageCount; i++)
    {
        m_FrameContext->m_CommandPools[i].Init(m_Device->GetLogicalDevice(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                               m_Device->GetGraphicsQueueFamilyIndex());
        m_FrameContext->m_CommandBuffers[i].Init(m_Device->GetLogicalDevice(), m_FrameContext->m_CommandPools[i].GetCommandPool(),
                                                 VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(m_Device->GetLogicalDevice(), &semaphoreInfo, nullptr, &m_FrameContext->m_ImagesAvailable[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_Device->GetLogicalDevice(), &semaphoreInfo, nullptr, &m_FrameContext->m_RenderFinished[i]) != VK_SUCCESS ||
            vkCreateFence(m_Device->GetLogicalDevice(), &fenceInfo, nullptr, &m_FrameContext->m_InFlightFences[i]) != VK_SUCCESS)
        {
            DBG_PRINTM("Failed to create semaphores!")
            assert(false);
        }
    }
    m_FrameContext->m_FrameCommandPool = new FVkCommandPool();
    m_FrameContext->m_FrameCommandPool->Init(m_Device->GetLogicalDevice(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, m_Device->GetGraphicsQueueFamilyIndex());


    m_DepthRenderTarget = new FVkTexture();
    VkExtent2D swapchainExtent = m_Swapchain->GetSwapchainExtent();
    createDepthTexture(*m_DepthRenderTarget, swapchainExtent.width, swapchainExtent.height, FindDepthFormat(m_Device->GetPhysicalDevice()),
                       m_MultisampledRenderTarget->GetSamplesCount(), 1);


    uint32_t vertexInputDescriptorSize = 0;
    uint32_t indexInputDescriptorSize = 0;

    if (pFrame.pPass->vertexInputInfo == Fleur::Graphics::EFLVertexInputDescription::VERTEX_INPUT_VERTEX_DATA)
        vertexInputDescriptorSize = sizeof(Fleur::Graphics::SVertexData);

    if (pFrame.pPass->indexInputInfo == Fleur::Graphics::EFLIndexInputDescription::INDEX_INPUT_UINT32)
        indexInputDescriptorSize = sizeof(uint32_t);
    else if (pFrame.pPass->indexInputInfo == Fleur::Graphics::EFLIndexInputDescription::INDEX_INPUT_UINT16)
        indexInputDescriptorSize = sizeof(uint16_t);

    m_VertexBuffer->Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                         1024u * 1024ul * 512ul, vertexInputDescriptorSize);

    m_IndexBuffer->Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                        1024u * 1024ul * 256ul, indexInputDescriptorSize);


    m_ImageSampler = createTextureSampler();
    createFallbackTexture(fallback);

    createStaticGeometryPass();

    updateStaticGeometryUboDescriptorSets(m_StaticGeometryDescriptorSetTextures, m_FallbackTextureIdx, m_TextureMap[m_FallbackTextureIdx].GetImageView(),
                                          m_ImageSampler);

    m_GeometryPipeline = createGeometryPipeline(pFrame.pPass->pVertexShaderInfo, pFrame.pPass->pFragmentShaderInfo, pFrame.pPass->inputAssemblyTopology,
                                                m_MultisampledRenderTarget->GetSamplesCount());

    m_DescriptorSetImageViewsToUpload.resize(swapChainImageCount);

    {
        FVkSingleTimeCommandBuffer frameCmd = FVkSingleTimeCommandBuffer(m_Device->GetLogicalDevice(), m_FrameContext->m_FrameCommandPool->GetCommandPool());

        frameCmd.TransitionImageLayout(m_MultisampledRenderTarget->GetTexture()->GetImage(), m_Swapchain->GetImageFormat(), VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, 1);

        frameCmd.Submit(m_Device->GetGraphicsQueue());
    }
}

vk::backend::impl::~impl()
{
    vkDeviceWaitIdle(m_Device->GetLogicalDevice());

    delete m_Skybox;

    uint32_t framebuffersCount = m_Swapchain->GetSwapchainImageCount();

    // 1. Synchronization objects
    for (size_t i = 0; i < framebuffersCount; i++)
    {
        vkDestroySemaphore(m_Device->GetLogicalDevice(), m_FrameContext->m_RenderFinished[i], nullptr);
        vkDestroySemaphore(m_Device->GetLogicalDevice(), m_FrameContext->m_ImagesAvailable[i], nullptr);
        vkDestroyFence(m_Device->GetLogicalDevice(), m_FrameContext->m_InFlightFences[i], nullptr);
    }

    // 2. CommandBuffer & CommandPool
    delete m_FrameContext;

    // 3. DescriptorSet & DescriptorPool & Descriptor set layout
    vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), m_DescriptorPool, nullptr);
    delete m_StaticGeometryTexturesDsl;
    delete m_StaticGeometryUboDsl;

    // 4. Pipeline
    delete m_GeometryPipeline;

    // 5. Swapchain & Framebuffers & swapchain image views

    // 7. All ImageViews
    delete m_MultisampledRenderTarget;
    delete m_FallbackCubemapTexture;
    delete m_DepthRenderTarget;

    m_TextureMap.clear();
    m_Swapchain->ReleaseSwapchainImageViews();

    // 8. Buffers
    delete m_VertexBuffer;
    delete m_IndexBuffer;
    for (size_t i = 0; i < m_UniformBuffers.size(); i++)
    {
        m_UniformBuffers[i].Unmap();
    }
    m_UniformBuffers.clear();

    // 9. Samplers
    vkDestroySampler(m_Device->GetLogicalDevice(), m_ImageSampler, nullptr);

    // 10. Swapchain
    delete m_Swapchain;

    // 15. VMA
    freeVma();

    // 11. Surface
    vkDestroySurfaceKHR(m_VulkanInstance, m_Surface, nullptr);

    // 12. LogicalDevice
    delete m_Device;

    // 13. Debug Utills & Validation Layers
    if (m_ValidationsEnabled)
        destroyDebugUtilsMessenger_EXT(m_VulkanInstance, debugMessenger, nullptr);

    // 14. Instance
    vkDestroyInstance(m_VulkanInstance, nullptr);


    // 16. Other
    delete m_GeometryVertexInput;
}


VkInstance vk::backend::impl::createInstance(bool enableValidation, const std::vector<const char*>& instanceExtensions,
                                             const std::vector<const char*>& validationLayers)
{
    m_ValidationsEnabled = enableValidation;

    uint32_t instanceVersion = 0;
    if (vkEnumerateInstanceVersion(&instanceVersion) != VK_SUCCESS)
        assert(false);
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

    if (vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance) != VK_SUCCESS)
    {
        assert(false);
    }

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

    if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &m_Surface) != VK_SUCCESS)
    {
        assert(false);
    }
#else
    VkWaylandSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.display = static_cast<struct wl_display**>(pNativeHandle)[0];
    createInfo.surface = static_cast<struct wl_surface**>(pNativeHandle)[1];

    if (vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &m_Surface) != VK_SUCCESS)
    {
        assert(false);
    }
#endif
    return m_Surface;
}


FVkPipeline* vk::backend::impl::createGeometryPipeline(Fleur::Graphics::SFLShaderInfo* pVertexInfo, Fleur::Graphics::SFLShaderInfo* pFragmentInfo,
                                                       Fleur::Graphics::EFLInputAssemblyTopology pInputAssemblyTopology, VkSampleCountFlagBits samplesCount)
{
    vk::ShaderCreateInfo shaderCreateInfo{.pVertexData = pVertexInfo->shaderCode,
                                          .vertexSize = pVertexInfo->sizeBytes,
                                          .pFragmentData = pFragmentInfo->shaderCode,
                                          .fragmentSize = pFragmentInfo->sizeBytes};


    auto& opaqueShader = m_ShaderMap.emplace(0, vk::FVkShader()).first->second;
    opaqueShader.Init(m_Device->GetLogicalDevice(), shaderCreateInfo);

    vk::GetPipelineInfo pipelineInfo{};
    pipelineInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    pipelineInfo.depthTestEnable = true;
    pipelineInfo.depthWriteEnable = true;
    pipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineInfo.samplesCount = samplesCount;
    pipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInfo.colorFormat = m_Swapchain->GetImageFormat();
    pipelineInfo.depthFormat = FindDepthFormat(m_Device->GetPhysicalDevice());

    FVkPipeline* pipeline = opaqueShader.GetPipeline(pipelineInfo);

    return pipeline;
}


VkShaderModule vk::backend::impl::createShaderModule(Fleur::Graphics::SFLShaderInfo* pShaderInfo)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = pShaderInfo->sizeBytes;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(pShaderInfo->shaderCode);

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_Device->GetLogicalDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create shader module!")
        assert(false);
    }

    return shaderModule;
}


void vk::backend::impl::createDescriptorPool()
{
    // ---------- descriptor indexing uses, 500k descriptors is min-limit ----------
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(m_Swapchain->GetSwapchainImageCount());

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = MAX_TEXTURES;

    VkDescriptorPoolCreateInfo poolInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
                                        .maxSets = m_Swapchain->GetSwapchainImageCount() + 1,
                                        .poolSizeCount = poolSizes.size(),
                                        .pPoolSizes = poolSizes.data()};

    if (vkCreateDescriptorPool(m_Device->GetLogicalDevice(), &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
    {
        DBG_PRINTM("failed to create descriptor pool!")
        assert(false);
    }
}
void vk::backend::impl::createDescriptorSets()
{
    // ---------- ubo ----------
    m_StaticGeometryDescriptorSetUbo.resize(m_Swapchain->GetSwapchainImageCount());
    auto uboDsl = m_StaticGeometryUboDsl->GetDescriptorSetLayout();

    for (size_t i = 0; i < m_StaticGeometryDescriptorSetUbo.size(); i++)
    {
        VkDescriptorSetAllocateInfo uniformBufferAllocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = m_DescriptorPool, .descriptorSetCount = 1, .pSetLayouts = &uboDsl};

        if (vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &uniformBufferAllocInfo, &m_StaticGeometryDescriptorSetUbo[i]) != VK_SUCCESS)
        {
            DBG_PRINTM("failed to allocate descriptor sets!")
            assert(true);
        }

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_UniformBuffers[i].GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(Fleur::Graphics::SFLGeometryUBO);

        VkWriteDescriptorSet descriptorWrites{};
        descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites.dstSet = m_StaticGeometryDescriptorSetUbo[i];
        descriptorWrites.dstBinding = 0;
        descriptorWrites.dstArrayElement = 0;
        descriptorWrites.descriptorCount = 1;
        descriptorWrites.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_Device->GetLogicalDevice(), 1, &descriptorWrites, 0, nullptr);
    }


    // ---------- textures ----------
    auto textureDsl = m_StaticGeometryTexturesDsl->GetDescriptorSetLayout();

    VkDescriptorSetAllocateInfo texturesDescriptorSetAllocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = m_DescriptorPool, .descriptorSetCount = 1, .pSetLayouts = &textureDsl};

    if (vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &texturesDescriptorSetAllocInfo, &m_StaticGeometryDescriptorSetTextures) != VK_SUCCESS)
    {
        DBG_PRINTM("failed to allocate descriptor sets!")
        assert(true);
    }

    VkImageView placeholderImageView = m_TextureMap[m_FallbackTextureIdx].GetImageView();
    VkDescriptorImageInfo imageSamplerInfo{};
    imageSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageSamplerInfo.imageView = placeholderImageView;
    imageSamplerInfo.sampler = m_ImageSampler;

    VkWriteDescriptorSet descriptorImageWrites{};
    descriptorImageWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorImageWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorImageWrites.dstSet = m_StaticGeometryDescriptorSetTextures;
    descriptorImageWrites.dstBinding = 0;
    descriptorImageWrites.dstArrayElement = 0;
    descriptorImageWrites.descriptorCount = 1;
    descriptorImageWrites.pImageInfo = &imageSamplerInfo;

    vkUpdateDescriptorSets(m_Device->GetLogicalDevice(), 1, &descriptorImageWrites, 0, nullptr);
}

void vk::backend::impl::updateUniformBuffer(uint32_t currentImage, Fleur::Graphics::SFLGeometryUBO* pUbo)
{
    pUbo->proj[1][1] *= -1;
    pUbo->model = glm::mat4(1.0f);
    m_UniformBuffers[currentFrame].MemCopy(pUbo, sizeof(*pUbo));
}


void vk::backend::impl::initializeVma()
{
    VmaAllocatorCreateInfo allocCreateInfo{};
    allocCreateInfo.instance = m_VulkanInstance;
    allocCreateInfo.physicalDevice = m_Device->GetPhysicalDevice();
    allocCreateInfo.device = m_Device->GetLogicalDevice();
    allocCreateInfo.vulkanApiVersion = VULKAN_VERSION;

    if (vmaCreateAllocator(&allocCreateInfo, &m_Allocator) != VkResult::VK_SUCCESS)
    {
        DBG_PRINTM("failed to initialize Vma!");
        assert(true);
    }
}
void vk::backend::impl::freeVma()
{
    vmaDestroyAllocator(m_Allocator);
}


// ---------- add draw ----------
void vk::backend::impl::addToDrawList(Fleur::Graphics::SFLModelView* pModelView)
{
    uint64_t globalIndexOffset = m_IndexBuffer->CurrentSize() / m_IndexBuffer->StrideBytes();
    uint64_t globalVertexOffset = m_VertexBuffer->CurrentSize() / m_VertexBuffer->StrideBytes();

    m_IndexBuffer->UploadDataToBuffer(pModelView->indecies.pData, pModelView->indecies.count);
    m_VertexBuffer->UploadDataToBuffer(pModelView->vertecies.pData, pModelView->vertecies.count);

    auto material = reinterpret_cast<const Fleur::Graphics::SFLMaterialView*>(pModelView->materials.pData);
    for (size_t i = 0; i < pModelView->meshes.count; i++)
    {
        auto& mesh = pModelView->meshes.pData[i];

        auto& draw = m_DrawList.emplace_back();

        draw.indexCount = mesh.indexCount;
        draw.vertexCount = mesh.vertexCount;

        draw.indexOffset = globalIndexOffset;
        draw.vertexOffset = globalVertexOffset;

        draw.material.albedo = pModelView->materials.pData[mesh.materialIdx].albedoID;

        globalIndexOffset += draw.indexCount;
    }
}
void vk::backend::impl::submitImageViews(Fleur::Graphics::SFLImageViewInfo* pInfo)
{
    for (size_t i = 0; i < pInfo->count; i++)
    {
        auto imageView = pInfo->pData + i;
        if (m_TextureMap.contains(imageView->ID))
            continue;

        VkFormat format = m_Device->GetTextureFormat(imageView->channels);
        VkImageAspectFlagBits aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        uint32_t layerSize = imageView->w * imageView->h * imageView->channels;
        uint32_t imageSize = layerSize * imageView->layerCount;
        uint32_t mimMapLevel = 1;
        if (imageView->layerCount == 1)
            mimMapLevel = CalculateMimMapLevel(imageView->w, imageView->h);

        auto& gpuTexture = m_TextureMap.emplace(imageView->ID, FVkTexture()).first->second;

        createTexture(*imageView, gpuTexture, format, aspect, mimMapLevel, imageView->layerCount);

        updateStaticGeometryUboDescriptorSets(m_StaticGeometryDescriptorSetTextures, imageView->ID, gpuTexture.GetImageView(), m_ImageSampler);
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
    if (vkCreateImageView(m_Device->GetLogicalDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create texture image view!");
        assert(true);
    }

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
    if (vkCreateSampler(m_Device->GetLogicalDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create texture sampler!");
        assert(true);
    }

    return sampler;
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
        FVkSingleTimeCommandBuffer frameCmd = FVkSingleTimeCommandBuffer(m_Device->GetLogicalDevice(), m_FrameContext->m_FrameCommandPool->GetCommandPool());
        frameCmd.TransitionImageLayout(cubemapImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cubemapAspect, cubemapMimMapCount,
                                       CUBEMAP_LAYERS_COUNT);
        frameCmd.CopyBufferToImage(stagingBuffer.GetBuffer(), cubemapImage, {view.w, view.h}, layerSize, CUBEMAP_LAYERS_COUNT);
        frameCmd.TransitionImageLayout(cubemapImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cubemapAspect,
                                       cubemapMimMapCount, CUBEMAP_LAYERS_COUNT);
        frameCmd.Submit(m_Device->GetGraphicsQueue());
    }
    m_FallbackCubemapTexture->CreateImaveView();

    delete buffer;
}

void vk::backend::impl::updateStaticGeometryUboDescriptorSets(VkDescriptorSet& set, uint32_t idx, VkImageView imageView, VkSampler& sampler)
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
        FVkSingleTimeCommandBuffer frameCmd = FVkSingleTimeCommandBuffer(m_Device->GetLogicalDevice(), m_FrameContext->m_FrameCommandPool->GetCommandPool());
        frameCmd.TransitionImageLayout(vkImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, aspect, mipLevels, layerCount);
        frameCmd.CopyBufferToImage(stagingBuffer.GetBuffer(), vkImage, {view.w, view.h}, layerSize, layerCount);
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

void vk::backend::impl::createDepthTexture(FVkTexture& texture, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits sampleCount,
                                           uint32_t mipMapCount)
{
    uint32_t layerCount = 1;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipMapCount;
    imageInfo.arrayLayers = layerCount;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = sampleCount;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkImage vkImage = texture.CreateImage(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          GetDepthAspect(format));
    texture.CreateImaveView();

    {
        FVkSingleTimeCommandBuffer frameCmd = FVkSingleTimeCommandBuffer(m_Device->GetLogicalDevice(), m_FrameContext->m_FrameCommandPool->GetCommandPool());
        frameCmd.TransitionImageLayout(vkImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, GetDepthAspect(format),
                                       mipMapCount, layerCount);
        frameCmd.Submit(m_Device->GetGraphicsQueue());
    }
}

void vk::backend::impl::startResize()
{
    m_WindowResizeIsInProgress = true;
    std::cout << "\nStartResize\n";
}
void vk::backend::impl::endResize(Fleur::SRect& rect)
{
    m_WindowResizeIsInProgress = false;
    m_Swapchain->OnWindowResized(rect);
    std::cout << "EndResize\n";
}

void vk::backend::impl::createSkybox(AssetID id, SFLShaderInfo* pVertexShaderInfo, SFLShaderInfo* pFragmentShaderInfo)
{
    assert(pVertexShaderInfo && pVertexShaderInfo->shaderCode);
    assert(pFragmentShaderInfo && pFragmentShaderInfo->shaderCode);

    if (m_Skybox)
        return;

    vk::ShaderCreateInfo shaderCreateInfo{.pVertexData = pVertexShaderInfo->shaderCode,
                                          .vertexSize = pVertexShaderInfo->sizeBytes,
                                          .pFragmentData = pFragmentShaderInfo->shaderCode,
                                          .fragmentSize = pFragmentShaderInfo->sizeBytes};


    auto& skyboxShader = m_ShaderMap.emplace(1, vk::FVkShader()).first->second;
    skyboxShader.Init(m_Device->GetLogicalDevice(), shaderCreateInfo);

    m_Skybox = new FVkSkybox();
    m_Skybox->Create(m_Device, m_Swapchain, m_FallbackCubemapTexture->GetImageView(), &skyboxShader, m_MultisampledRenderTarget->GetSamplesCount(),
                     FindDepthFormat(m_Device->GetPhysicalDevice()));
}
void vk::backend::impl::setSkybox(AssetID id)
{
    m_Skybox->SetSkybox(m_TextureMap[id].GetImageView());
}

vk::FVkShader* vk::backend::impl::AddShader(ShaderCreateInfo& shaderInfo)
{
    return nullptr;
}

void vk::backend::impl::BeginRendering(VkCommandBuffer cmd, VkRect2D renderarea, uint32_t currentImage)
{
    VkClearValue clearColor{
        .color = {1.0f, 1.0f, 1.0f, 1.0f},
    };
    VkClearValue clearDepth{
        .depthStencil = {.depth = 1.0f, .stencil = 0},
    };

    VkRenderingAttachmentInfoKHR colorAttachment{.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                                                 .pNext = nullptr,
                                                 .imageView = m_Swapchain->GetSwapchainImageView(currentImage),
                                                 .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                 .resolveMode = VK_RESOLVE_MODE_NONE,
                                                 .resolveImageView = nullptr,
                                                 .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                                 .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                 .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                                 .clearValue = clearColor};
    VkSampleCountFlagBits sampleCount = m_MultisampledRenderTarget->GetSamplesCount();
    if (sampleCount > VK_SAMPLE_COUNT_1_BIT)
    {
        colorAttachment.imageView = m_MultisampledRenderTarget->GetTexture()->GetImageView();
        colorAttachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
        colorAttachment.resolveImageView = m_Swapchain->GetSwapchainImageView(currentImage);
        colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }

    VkRenderingAttachmentInfoKHR depthAttachment{.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                                                 .pNext = nullptr,
                                                 .imageView = m_DepthRenderTarget->GetImageView(),
                                                 .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                 .resolveMode = VK_RESOLVE_MODE_NONE,
                                                 .resolveImageView = nullptr,
                                                 .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                                 .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                 .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                                 .clearValue = clearDepth};

    VkRenderingInfoKHR renderingInfo{.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
                                     .renderArea = renderarea,
                                     .layerCount = 1,
                                     .viewMask = 0,
                                     .colorAttachmentCount = 1,
                                     .pColorAttachments = &colorAttachment,
                                     .pDepthAttachment = &depthAttachment};

    vkCmdBeginRendering(cmd, &renderingInfo);
}

void vk::backend::impl::createStaticGeometryPass()
{
    m_StaticGeometryUboDsl =
        FVkDescriptorSetLayout::Builder(m_Device->GetLogicalDevice()).add(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1).build();

    m_StaticGeometryTexturesDsl = FVkDescriptorSetLayout::Builder(m_Device->GetLogicalDevice())
                                      .add(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, MAX_TEXTURES)
                                      .build();

    createDescriptorPool();

    VkDeviceSize bufferSize = sizeof(Fleur::Graphics::SFLGeometryUBO);

    m_UniformBuffers.resize(m_Swapchain->GetSwapchainImageCount());

    for (size_t i = 0; i < m_Swapchain->GetSwapchainImageCount(); i++)
    {
        m_UniformBuffers[i].Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, bufferSize, bufferSize);
    }

    createDescriptorSets();
}

void vk::backend::impl::update(Fleur::Graphics::SFLCameraData cameraData)
{
    if (m_WindowResizeIsInProgress)
    {
        return;
    }
    if (!m_Swapchain->ReadyToPresent())
    {
        vkDeviceWaitIdle(m_Device->GetLogicalDevice());
        m_Swapchain->Recreate(m_Surface, m_Device->GetPresentQueueFamilyIndex(), m_MultisampledRenderTarget->GetTexture()->GetImageView(),
                              m_DepthRenderTarget->GetImageView());
    }


    vkWaitForFences(m_Device->GetLogicalDevice(), 1, &m_FrameContext->m_InFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_Device->GetLogicalDevice(), 1, &m_FrameContext->m_InFlightFences[currentFrame]);

    uint32_t imageIndex;
    VkResult isSwapchainValid{};
    isSwapchainValid = vkAcquireNextImageKHR(m_Device->GetLogicalDevice(), m_Swapchain->GetSwapchain(), UINT64_MAX,
                                             m_FrameContext->m_ImagesAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (isSwapchainValid == VK_ERROR_OUT_OF_DATE_KHR || isSwapchainValid == VK_SUBOPTIMAL_KHR)
    {
        assert(false);
        std::cout << "VK_ERROR_OUT_OF_DATE_KHR\n";
    }
    else if (isSwapchainValid != VK_SUCCESS && isSwapchainValid != VK_SUBOPTIMAL_KHR)
    {
        DBG_PRINTM("Failed to present swap chain image!")
        assert(false);
    }

    Fleur::Graphics::SFLGeometryUBO ubo{glm::mat4(1.0f), cameraData.view, cameraData.proj};
    updateUniformBuffer(currentFrame, &ubo);

    // ---------- issuing commands ----------
    vkResetCommandPool(m_Device->GetLogicalDevice(), m_FrameContext->m_CommandPools[currentFrame].GetCommandPool(), 0);

    auto& cmd = m_FrameContext->m_CommandBuffers[currentFrame];
    cmd.Begin();

    transitionImageLayout(*cmd.GetCommandBuffer(), m_Swapchain->GetSwapchainImage(imageIndex), m_Swapchain->GetImageFormat(), VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    VkRect2D renderArea{
        .offset = {0, 0},
        .extent = {.width = m_Swapchain->GetSwapchainExtent().width, .height = m_Swapchain->GetSwapchainExtent().height},
    };
    BeginRendering(*m_FrameContext->m_CommandBuffers[currentFrame].GetCommandBuffer(), renderArea, imageIndex);

    if (m_Skybox)
    {
        m_Skybox->Record(*cmd.GetCommandBuffer(), m_Swapchain->GetSwapchainExtent(), cameraData);
    }

    cmd.BindVertexBuffer(&m_VertexBuffer->GetBuffer());
    if (m_IndexBuffer->StrideBytes() == 4)
        cmd.BindIndexBuffer(&m_IndexBuffer->GetBuffer(), VK_INDEX_TYPE_UINT32);
    else if (m_IndexBuffer->StrideBytes() == 2)
        cmd.BindIndexBuffer(&m_IndexBuffer->GetBuffer(), VK_INDEX_TYPE_UINT16);

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

    std::array<VkDescriptorSet, 2> dst{m_StaticGeometryDescriptorSetUbo[currentFrame], m_StaticGeometryDescriptorSetTextures};
    vkCmdBindDescriptorSets(*cmd.GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_GeometryPipeline->GetPipelineLayout(), 0, dst.size(), dst.data(), 0,
                            nullptr);

    for (const auto& draw : m_DrawList)
    {
        SFLPushConstant pushConstant{.albedoIdx = draw.material.albedo};
        cmd.PushConstant(m_GeometryPipeline->GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, pushConstant);
        cmd.DrawIndexed(draw.indexCount, draw.indexOffset, draw.vertexOffset);
    }
    cmd.EndRendering();
    transitionImageLayout(*cmd.GetCommandBuffer(), m_Swapchain->GetSwapchainImage(imageIndex), m_Swapchain->GetImageFormat(),
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    cmd.End();


    VkSemaphore waitSemaphores[] = {m_FrameContext->m_ImagesAvailable[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;

    submitInfo.pCommandBuffers = m_FrameContext->m_CommandBuffers[currentFrame].GetCommandBuffer();

    VkSemaphore signalSemaphores[] = {m_FrameContext->m_RenderFinished[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_FrameContext->m_InFlightFences[currentFrame]) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to submit draw command buffer!")
        assert(false);
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.pResults = nullptr;  // Optional

    VkSwapchainKHR swapChains[] = {m_Swapchain->GetSwapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);

    currentFrame = (currentFrame + 1) % m_Swapchain->GetSwapchainImageCount();
}