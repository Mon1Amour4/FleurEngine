
// This entire .cpp file was so big so it was pain in the ass to navigate throughout
// I've hidden vulkanBackendImpl declaration into .hpp file
#if defined(FLEUR_PLATFORM_WIN)
#define NOMINMAX
#define NOGDI
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "PrivateVulkanImpl.hpp"


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

void vk::backend::RegisterModel(AssetID model, const SVertexData* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount,
                                const FLDrawItem* primitives, uint32_t primitiveCount)
{
    pImpl->registerModel(model, vertices, vertexCount, indices, indexCount, primitives, primitiveCount);
}
void vk::backend::UnregisterModel(AssetID model)
{
    pImpl->unregisterModel(model);
}
void vk::backend::RemoveTexture(AssetID texture)
{
    // TODO: free the bindless texture slot (slot free-list). Stubbed for now.
}
void vk::backend::BeginFrame(Fleur::Graphics::SFLCameraData& cameraData)
{
    pImpl->beginFrame(cameraData);
}
void vk::backend::Draw(AssetID model, const glm::mat4& transform)
{
    pImpl->drawModel(model, transform);
}
void vk::backend::EndFrame()
{
    pImpl->endFrame();
}
void vk::backend::DrawLine(glm::vec3 a, glm::vec3 b, glm::vec3 color, bool depthTest)
{
    pImpl->m_DebugDraw->AddLine(a, b, color);
}
void vk::backend::DrawPoint(glm::vec3 p, glm::vec3 color, float size, bool depthTest)
{
    pImpl->m_DebugDraw->AddPoint(p, color, size);
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

    m_MultisampledRenderTarget = new FVkMultisampler();
    m_MultisampledRenderTarget->Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_SAMPLE_COUNT_1_BIT,
                                     m_Swapchain->GetSwapchainExtent().width, m_Swapchain->GetSwapchainExtent().height, m_Swapchain->GetImageFormat());

    m_FramesInFlight = m_Swapchain->GetSwapchainImageCount();
    m_FramesInFlight = 3;

    m_FrameContext = new FrameContext(m_FramesInFlight);
    for (size_t i = 0; i < m_FramesInFlight; i++)
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

    vertexInputDescriptorSize = sizeof(Fleur::Graphics::SVertexData);

    indexInputDescriptorSize = sizeof(uint32_t);

    m_VertexBuffer->Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                         1024u * 1024ul * 512ul, vertexInputDescriptorSize);

    m_IndexBuffer->Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                        1024u * 1024ul * 256ul, indexInputDescriptorSize);


    m_ImageSampler = createTextureSampler();
    createFallbackTexture(fallback);

    createStaticGeometryPass();

    updateStaticGeometryUboDescriptorSets(m_StaticGeometryDescriptorSetTextures, m_FallbackTextureIdx, m_TextureMap[m_FallbackTextureIdx].GetImageView(),
                                          m_ImageSampler);

    m_DescriptorSetImageViewsToUpload.resize(m_FramesInFlight);

    {
        FVkSingleTimeCommandBuffer frameCmd = FVkSingleTimeCommandBuffer(m_Device->GetLogicalDevice(), m_FrameContext->m_FrameCommandPool->GetCommandPool());

        frameCmd.TransitionImageLayout(m_MultisampledRenderTarget->GetTexture()->GetImage(), m_Swapchain->GetImageFormat(), VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, 1);

        frameCmd.Submit(m_Device->GetGraphicsQueue());
    }

    m_DebugDraw = new FVkDebugDraw();
}

vk::backend::impl::~impl()
{
    vkDeviceWaitIdle(m_Device->GetLogicalDevice());

    delete m_Skybox;
    delete m_DebugDraw;

    uint32_t framebuffersCount = m_FramesInFlight;

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
    delete m_TransparentPipeline;

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
#endif

    return m_Surface;
}


FVkPipeline* vk::backend::impl::createGeometryPipeline(Fleur::Graphics::SFLShaderInfo pVertexInfo, Fleur::Graphics::SFLShaderInfo pFragmentInfo,
                                                       VkSampleCountFlagBits samplesCount)
{
    vk::ShaderCreateInfo shaderCreateInfo{.pVertexData = pVertexInfo.shaderCode,
                                          .vertexSize = pVertexInfo.sizeBytes,
                                          .pFragmentData = pFragmentInfo.shaderCode,
                                          .fragmentSize = pFragmentInfo.sizeBytes};


    auto& opaqueShader = m_ShaderMap.emplace("opaque", vk::FVkShader()).first->second;
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

FVkPipeline* vk::backend::impl::createTransparentPipeline(Fleur::Graphics::SFLShaderInfo pVertexInfo, Fleur::Graphics::SFLShaderInfo pFragmentInfo,
                                                          VkSampleCountFlagBits samplesCount)
{
    vk::ShaderCreateInfo shaderCreateInfo{.pVertexData = pVertexInfo.shaderCode,
                                          .vertexSize = pVertexInfo.sizeBytes,
                                          .pFragmentData = pFragmentInfo.shaderCode,
                                          .fragmentSize = pFragmentInfo.sizeBytes};


    auto& opaqueShader = m_ShaderMap.emplace("opaque", vk::FVkShader()).first->second;
    if (!opaqueShader.isInitialized())
        opaqueShader.Init(m_Device->GetLogicalDevice(), shaderCreateInfo);

    vk::GetPipelineInfo pipelineInfo{};
    pipelineInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    pipelineInfo.depthTestEnable = true;
    pipelineInfo.depthWriteEnable = false;
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
    poolSizes[0].descriptorCount = static_cast<uint32_t>(m_FramesInFlight);

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = MAX_TEXTURES;

    VkDescriptorPoolCreateInfo poolInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
                                        .maxSets = m_FramesInFlight + 1,
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
    m_StaticGeometryDescriptorSetUbo.resize(m_FramesInFlight);
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
    // proj is already Vulkan-flipped once in m_CameraData (beginFrame) — no per-consumer flip.
    pUbo->model = glm::mat4(1.0f);
    m_UniformBuffers[m_CurrentFrame].MemCopy(pUbo, sizeof(*pUbo));
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


void vk::backend::impl::uploadTextures(Fleur::Graphics::SFLImageViewInfo* pInfo)
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
    std::cout << "\EndResize\n";
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
                     FindDepthFormat(m_Device->GetPhysicalDevice()));
}
void vk::backend::impl::setSkybox(AssetID id)
{
    m_Skybox->SetSkybox(m_TextureMap[id].GetImageView());
}

void vk::backend::impl::createPass(EFLPassKind kind, SFLShaderStages shaderStages)
{
    if (kind == EFLPassKind::Opaque)
    {
        // TODO if pipeline already exists, need to release it
        m_GeometryPipeline = createGeometryPipeline(shaderStages.vertex, shaderStages.fragment, m_MultisampledRenderTarget->GetSamplesCount());
    }
    else if (kind == EFLPassKind::Transparent)
    {
        m_TransparentPipeline = createTransparentPipeline(shaderStages.vertex, shaderStages.fragment, m_MultisampledRenderTarget->GetSamplesCount());
    }
    else if (kind == EFLPassKind::AABB_DEBUG)
    {
        if (!m_DebugDraw->IsInitialized())
        {
            vk::ShaderCreateInfo shaderCreateInfo{.pVertexData = shaderStages.vertex.shaderCode,
                                                  .vertexSize = shaderStages.vertex.sizeBytes,
                                                  .pFragmentData = shaderStages.fragment.shaderCode,
                                                  .fragmentSize = shaderStages.fragment.sizeBytes};


            auto& debugShader = m_ShaderMap.emplace("Debug", vk::FVkShader()).first->second;
            if (!debugShader.isInitialized())
                debugShader.Init(m_Device->GetLogicalDevice(), shaderCreateInfo);

            m_DebugDraw->Create(m_Device, m_Swapchain, &debugShader, m_MultisampledRenderTarget->GetSamplesCount(),
                                FindDepthFormat(m_Device->GetPhysicalDevice()), m_FramesInFlight);
        }
    }
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

    m_UniformBuffers.resize(m_FramesInFlight);

    for (size_t i = 0; i < m_FramesInFlight; i++)
    {
        m_UniformBuffers[i].Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, bufferSize, bufferSize);
    }

    createDescriptorSets();
}


bool vk::backend::impl::beginFrame(Fleur::Graphics::SFLCameraData& cameraData)
{
    if (m_WindowResizeIsInProgress)
        return false;

    if (!m_Swapchain->ReadyToPresent())
    {
        vkDeviceWaitIdle(m_Device->GetLogicalDevice());
        m_Swapchain->Recreate(m_Surface, m_Device->GetPresentQueueFamilyIndex(), m_MultisampledRenderTarget->GetTexture()->GetImageView(),
                              m_DepthRenderTarget->GetImageView());
    }

    vkWaitForFences(m_Device->GetLogicalDevice(), 1, &m_FrameContext->m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_Device->GetLogicalDevice(), 1, &m_FrameContext->m_InFlightFences[m_CurrentFrame]);

    VkResult isSwapchainValid = vkAcquireNextImageKHR(m_Device->GetLogicalDevice(), m_Swapchain->GetSwapchain(), UINT64_MAX,
                                                      m_FrameContext->m_ImagesAvailable[m_CurrentFrame], VK_NULL_HANDLE, &m_ImageIndex);
    if (isSwapchainValid == VK_ERROR_OUT_OF_DATE_KHR || isSwapchainValid == VK_SUBOPTIMAL_KHR)
    {
        assert(false);
    }
    else if (isSwapchainValid != VK_SUCCESS && isSwapchainValid != VK_SUBOPTIMAL_KHR)
    {
        DBG_PRINTM("Failed to present swap chain image!")
        assert(false);
    }

    m_CameraData = cameraData;
    m_CameraData.proj[1][1] *= -1;  // Vulkan Y-flip — single source for all VK passes (geometry/skybox/debug)

    Fleur::Graphics::SFLGeometryUBO ubo{glm::mat4(1.0f), m_CameraData.view, m_CameraData.proj};
    updateUniformBuffer(m_CurrentFrame, &ubo);

    vkResetCommandPool(m_Device->GetLogicalDevice(), m_FrameContext->m_CommandPools[m_CurrentFrame].GetCommandPool(), 0);

    if (!m_GeometryPipeline)
        return false;

    auto& cmd = m_FrameContext->m_CommandBuffers[m_CurrentFrame];
    cmd.Begin();

    transitionImageLayout(*cmd.GetCommandBuffer(), m_Swapchain->GetSwapchainImage(m_ImageIndex), m_Swapchain->GetImageFormat(), VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    VkRect2D renderArea{
        .offset = {0, 0},
        .extent = {.width = m_Swapchain->GetSwapchainExtent().width, .height = m_Swapchain->GetSwapchainExtent().height},
    };
    BeginRendering(*cmd.GetCommandBuffer(), renderArea, m_ImageIndex);

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

    std::array<VkDescriptorSet, 2> dst{m_StaticGeometryDescriptorSetUbo[m_CurrentFrame], m_StaticGeometryDescriptorSetTextures};
    cmd.BindDescriptorSets(m_GeometryPipeline->GetPipelineLayout(), dst.data(), dst.size());

    return true;
}

void vk::backend::impl::endFrame()
{
    auto& cmd = m_FrameContext->m_CommandBuffers[m_CurrentFrame];

    // After opaque (recorded via Draw), before transparent: skybox fills the
    // background only where no geometry was written.
    if (m_Skybox)
        m_Skybox->Record(*cmd.GetCommandBuffer(), m_Swapchain->GetSwapchainExtent(), m_CameraData);

    if (m_TransparentDraws.size() > 0)
    {
        // Skybox rebound its own vertex buffer above — restore the geometry buffers.
        cmd.BindVertexBuffer(&m_VertexBuffer->GetBuffer());
        cmd.BindIndexBuffer(&m_IndexBuffer->GetBuffer(), VK_INDEX_TYPE_UINT32);
        cmd.BindPipeline(m_TransparentPipeline->GetPipeline());
        for (auto& it : m_TransparentDraws)
        {
            SFLPushConstant pushConstant = MakePush(it);
            cmd.PushConstant(m_GeometryPipeline->GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, pushConstant);
            cmd.DrawIndexed(it.indexCount, it.indexOffset, it.vertexOffset);
        }
    }

    m_DebugDraw->Record(cmd, m_CameraData, m_CurrentFrame);
    m_DebugDraw->Clear();

    cmd.EndRendering();
    transitionImageLayout(*cmd.GetCommandBuffer(), m_Swapchain->GetSwapchainImage(m_ImageIndex), m_Swapchain->GetImageFormat(),
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    cmd.End();

    VkSemaphore waitSemaphores[] = {m_FrameContext->m_ImagesAvailable[m_CurrentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = cmd.GetCommandBuffer();

    VkSemaphore signalSemaphores[] = {m_FrameContext->m_RenderFinished[m_CurrentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_FrameContext->m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to submit draw command buffer!")
        assert(false);
    }

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
                                      const FLDrawItem* primitives, uint32_t primitiveCount)
{
    uint64_t globalIndexOffset = m_IndexBuffer->CurrentSize() / m_IndexBuffer->StrideBytes();
    uint64_t globalVertexOffset = m_VertexBuffer->CurrentSize() / m_VertexBuffer->StrideBytes();

    m_VertexBuffer->UploadDataToBuffer(vertices, verticesCount);
    m_IndexBuffer->UploadDataToBuffer(indices, indexCount);

    auto& list = m_RegisteredModels[id];
    for (uint32_t i = 0; i < primitiveCount; i++)
    {
        const auto& item = primitives[i];
        auto& draw = list.emplace_back();

        draw.FromMaterial(item.material);

        draw.indexCount = item.indexCount;
        draw.indexOffset = globalIndexOffset + item.indexStart;
        draw.vertexOffset = globalVertexOffset;
        draw.bucket = item.material.mode;
    }
}

void vk::backend::impl::unregisterModel(AssetID id)
{
    m_RegisteredModels.erase(id);
    // TODO: reclaim geometry buffer space (bump allocator has no free; needs a sub-allocator).
}

void vk::backend::impl::drawModel(AssetID id, const glm::mat4& /*transform*/)
{
    auto it = m_RegisteredModels.find(id);
    if (it == m_RegisteredModels.end())
        return;

    // m_TransparentDraws.clear();
    //  TODO: per-draw transform via push-constant (needs vertex shader change). Identity for now.
    auto& cmd = m_FrameContext->m_CommandBuffers[m_CurrentFrame];
    for (const auto& draw : it->second)
    {
        if (draw.bucket == FLAlphaMode::FL_OPAQUE || draw.bucket == FLAlphaMode::FL_MASK)
        {
            SFLPushConstant pushConstant = MakePush(draw);
            cmd.PushConstant(m_GeometryPipeline->GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, pushConstant);
            cmd.DrawIndexed(draw.indexCount, draw.indexOffset, draw.vertexOffset);
        }
        else
        {
            m_TransparentDraws.push_back(draw);
            /*std::sort(m_TransparentDraws.begin(), m_TransparentDraws.end(),
                      [](const DrawInfo& a, const DrawInfo& b) { return a.material.alphaCutoff < b.material.alphaCutoff; });*/
        }
    }
}
