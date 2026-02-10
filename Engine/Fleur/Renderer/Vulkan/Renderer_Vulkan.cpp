
// This entire .cpp file was so big so it was pain in the ass to navigate throughout
// I've hidden vulkanBackendImpl declaration into .hpp file
#if defined(FLEUR_PLATFORM_WIN)
#define NOMINMAX
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "PrivateVulkanImpl.hpp"


vulkanBackend::vulkanBackend(bool enableValidation, Fleur::Graphics::SFLFrame& pFrame, void* pNativeHandle, Fleur::SRect& framebufferSize,
                             Fleur::Graphics::SFLImageView& fallback)
    : pImpl(new vulkanBackendImpl(enableValidation, pFrame, pNativeHandle, framebufferSize, fallback))
{
}
vulkanBackend::~vulkanBackend()
{
    delete pImpl;
}
void vulkanBackend::AddToDrawList(Fleur::Graphics::SFLModelView* pModelView)
{
    pImpl->AddToDrawList(pModelView);
}
void vulkanBackend::Update(Fleur::Graphics::SFLGeometryUBO* pUbo)
{
    pImpl->update(pUbo);
}
void vulkanBackend::SubmitImageViews(Fleur::Graphics::SFLImageViewInfo* pInfo)
{
    pImpl->SubmitImageViews(pInfo);
}

void vulkanBackend::CreateSkybox(AssetID id, SFLShaderInfo* pVertexShaderInfo, SFLShaderInfo* pFragmentShaderInfo)
{
    pImpl->CreateSkybox(id, pVertexShaderInfo, pFragmentShaderInfo);
}

void vulkanBackend::SetSkybox(AssetID id)
{
    pImpl->SetSkybox(id);
}

void vulkanBackend::StartResize()
{
    pImpl->StartResize();
}
void vulkanBackend::EndResize(Fleur::SRect& rect)
{
    pImpl->EndResize(rect);
}


//======================================================================
// vulkanBackend::vulkanBackendImpl
vulkanBackend::vulkanBackendImpl::vulkanBackendImpl(bool enableValidation, Fleur::Graphics::SFLFrame& pFrame, void* pNativeHandle,
                                                    Fleur::SRect& framebufferSize, Fleur::Graphics::SFLImageView& fallback)
    : m_WindowResizeIsInProgress(false)
{
    m_Capabilities = new FVkCapabilities(enableValidation);

    m_VulkanInstance = createInstance();
    setupDebugMessenger();

    m_Swapchain = new FVkSwapchain();
    m_Surface = CreateSurface(m_VulkanInstance, pNativeHandle);

    SDeviceInfo deviceInfo{};
    deviceInfo.presentationSupport = true;
    deviceInfo.neededQueueFamilyFlags = VK_QUEUE_GRAPHICS_BIT;
    deviceInfo.surface = m_Surface;
    deviceInfo.requiredDeviceExtensions = deviceExtensions;

    m_Device = FVkDevice::CreateSuitableDevice(m_VulkanInstance, deviceInfo);
    m_Device->CreateLogicalDevice(deviceExtensions);

    initializeVma();
    m_Swapchain->CreateSwapchain(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Surface,
                                 {framebufferSize.x, framebufferSize.y, framebufferSize.width, framebufferSize.height},
                                 m_Device->GetGraphicsQueueFamilyIndex());

    m_VertexBuffer = new FVkBuffer();
    m_IndexBuffer = new FVkBuffer();

    m_GeometryVertexInput = new SFLVertexInput();
    m_GeometryVertexInput->RegisterAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Fleur::Graphics::SVertexData, Position));
    m_GeometryVertexInput->RegisterAttribute(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Fleur::Graphics::SVertexData, TexCoord));
    m_GeometryVertexInput->RegisterAttribute(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Fleur::Graphics::SVertexData, Normal));

    m_Multisampler = new FVkMultisampler();
    m_Multisampler->Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Swapchain->GetSwapchainExtent().width,
                         m_Swapchain->GetSwapchainExtent().height, m_Swapchain->GetImageFormat());

    CreateGeometryRenderPass();

    createDescriptorSetLayout();

    m_GeometryPipeline = CreateGeometryPipeline(pFrame.pPass->pVertexShaderInfo, pFrame.pPass->pFragmentShaderInfo, pFrame.pPass->inputAssemblyTopology,
                                                m_Multisampler->GetSamplesCount());


    m_GraphicsCommandPool = new FVkCommandPool();
    m_GraphicsCommandPool->Init(m_Device->GetLogicalDevice(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_Device->GetGraphicsQueueFamilyIndex());

    m_Device->CreateFrameCommandBuffer(m_GraphicsCommandPool->GetCommandPool());

    m_Depth.depthTexture = new FVkTexture();
    CreateDepthBuffer(m_Depth, m_Device->GetPhysicalDevice(), m_Multisampler->GetSamplesCount(), 1);

    m_Swapchain->CreateFrameBuffers(m_GeometryRenderPass, m_Multisampler->GetTexture()->GetImageView(), m_Depth.depthTexture->GetImageView());

    m_DescriptorSetImageViews.resize(m_Swapchain->GetSwapchainFramebuffersCount());
    m_PrimaryCmdBuffers.resize(m_Swapchain->GetSwapchainFramebuffersCount());
    m_SecondaryCmdBuffers.resize(m_Swapchain->GetSwapchainFramebuffersCount());
    m_SecondaryCmdValidation.resize(m_SecondaryCmdBuffers.size());

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

    createUniformBuffers();
    createDescriptorPool();

    CreateFallbackTexture(fallback);
    m_ImageSampler = createTextureSampler();

    createDescriptorSets();

    for (size_t i = 0; i < m_PrimaryCmdBuffers.size(); i++)
    {
        m_PrimaryCmdBuffers[i].Init(m_Device->GetLogicalDevice(), m_GraphicsCommandPool->GetCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    }
    m_SkyboxCmd.Init(m_Device->GetLogicalDevice(), m_GraphicsCommandPool->GetCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    for (size_t i = 0; i < m_SecondaryCmdBuffers.size(); i++)
    {
        m_SecondaryCmdBuffers[i].Init(m_Device->GetLogicalDevice(), m_GraphicsCommandPool->GetCommandPool(), VK_COMMAND_BUFFER_LEVEL_SECONDARY);
        UpdateGeometrySecondaryCmdBuffer(i);
        InitGeometryPrimaryCmdBuffers(i);
    }

    createSyncObjects();
}
vulkanBackend::vulkanBackendImpl::~vulkanBackendImpl()
{
    vkDeviceWaitIdle(m_Device->GetLogicalDevice());

    delete m_Skybox;

    uint32_t framebuffersCount = m_Swapchain->GetSwapchainFramebuffersCount();

    // 1. Synchronization objects
    for (size_t i = 0; i < framebuffersCount; i++)
    {
        vkDestroySemaphore(m_Device->GetLogicalDevice(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_Device->GetLogicalDevice(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_Device->GetLogicalDevice(), inFlightFences[i], nullptr);
    }

    // 2. CommandBuffer & CommandPool
    m_PrimaryCmdBuffers.clear();
    m_SecondaryCmdBuffers.clear();
    delete m_GraphicsCommandPool;

    // 3. DescriptorSet & DescriptorPool & Descriptor set layout
    vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_Device->GetLogicalDevice(), m_GeometryDSL, nullptr);

    // 4. Pipeline
    delete m_GeometryPipeline;

    // 5. Swapchain & Framebuffers & swapchain image views


    // 5. Framebuffers
    m_Swapchain->ReleaseFramebuffers();

    // 6. RenderPass
    vkDestroyRenderPass(m_Device->GetLogicalDevice(), m_GeometryRenderPass, nullptr);

    // 7. All ImageViews
    delete m_Multisampler;
    delete m_FallbackTexture;
    delete m_Depth.depthTexture;
    // delete m_Depth.depthTexture;
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
    if (m_Capabilities->ValidationEnabled())
        DestroyDebugUtilsMessengerEXT(m_VulkanInstance, debugMessenger, nullptr);

    // 14. Instance
    vkDestroyInstance(m_VulkanInstance, nullptr);


    // 16. Other
    delete m_GeometryVertexInput;
    delete m_Capabilities;
}


//======================================================================
// VkInstance
VkInstance vulkanBackend::vulkanBackendImpl::createInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Fleur Engine";
    appInfo.applicationVersion = VULKAN_VERSION;
    appInfo.pEngineName = "Fleur Engine";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    appInfo.apiVersion = VULKAN_VERSION;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

    if (m_Capabilities->ValidationEnabled())
    {
        m_Capabilities->EnableValidationLayersSupport(createInfo);

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }
    m_Capabilities->EnableExtensions(createInfo);

    if (vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance) != VK_SUCCESS)
    {
        assert(false);
    }

    return m_VulkanInstance;
}

void vulkanBackend::vulkanBackendImpl::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}
void vulkanBackend::vulkanBackendImpl::setupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(m_VulkanInstance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to set up debug messenger");
    }
}
VkResult vulkanBackend::vulkanBackendImpl::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                                        const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
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

void vulkanBackend::vulkanBackendImpl::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                                     const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}


//======================================================================
// VkRenderPass
void vulkanBackend::vulkanBackendImpl::CreateGeometryRenderPass()
{
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = FindDepthFormat(m_Device->GetPhysicalDevice());
    depthAttachment.samples = m_Multisampler->GetSamplesCount();
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_Swapchain->GetImageFormat();
    colorAttachment.samples = m_Multisampler->GetSamplesCount();
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  // Not VK_IMAGE_LAYOUT_PRESENT_SRC_KHR cause of multisampling

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = m_Swapchain->GetImageFormat();
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;


    if (vkCreateRenderPass(m_Device->GetLogicalDevice(), &renderPassInfo, nullptr, &m_GeometryRenderPass) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create render pass!")
        assert(false);
    }
}


VkSurfaceKHR vulkanBackend::vulkanBackendImpl::CreateSurface(VkInstance instance, void* pNativeHandle)
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

//======================================================================
// VkPipeline
FVkPipeline* vulkanBackend::vulkanBackendImpl::CreateGeometryPipeline(Fleur::Graphics::SFLShaderInfo* pVertexInfo,
                                                                      Fleur::Graphics::SFLShaderInfo* pFragmentInfo,
                                                                      Fleur::Graphics::EFLInputAssemblyTopology pInputAssemblyTopology,
                                                                      VkSampleCountFlagBits samplesCount)
{
    VkShaderModule vertexShaderModule = CreateShaderModule(pVertexInfo);
    VkShaderModule vertexFragmentModule = CreateShaderModule(pFragmentInfo);

    SFPipelineCreationInfo pipelineInfo{};
    pipelineInfo.device = m_Device->GetLogicalDevice();
    pipelineInfo.renderPass = m_GeometryRenderPass;
    pipelineInfo.descriptorSetLayout = m_GeometryDSL;
    pipelineInfo.fragmentShader = vertexFragmentModule;
    pipelineInfo.vertexShader = vertexShaderModule;
    pipelineInfo.pushConstantSize = sizeof(uint32_t);
    pipelineInfo.vertexInput = m_GeometryVertexInput;
    pipelineInfo.samplesCount = samplesCount;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_Swapchain->GetSwapchainExtent().width;
    viewport.height = (float)m_Swapchain->GetSwapchainExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    pipelineInfo.viewport = &viewport;
    pipelineInfo.extent = m_Swapchain->GetSwapchainExtent();
    pipelineInfo.topology = pInputAssemblyTopology == Fleur::Graphics::FL_INPUT_ASSEMBLY_TOPOLOGY_TRIANGLE_LIST ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
                                                                                                                : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInfo.depthWriteEnable = true;

    FVkPipeline* geometryPipeline = new FVkPipeline();
    geometryPipeline->Init(&pipelineInfo);

    vkDestroyShaderModule(m_Device->GetLogicalDevice(), vertexShaderModule, nullptr);
    vkDestroyShaderModule(m_Device->GetLogicalDevice(), vertexFragmentModule, nullptr);

    return geometryPipeline;
}


//======================================================================
// VkShaderModule
VkShaderModule vulkanBackend::vulkanBackendImpl::CreateShaderModule(Fleur::Graphics::SFLShaderInfo* pShaderInfo)
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


void vulkanBackend::vulkanBackendImpl::InitGeometryPrimaryCmdBuffers(uint32_t idx)
{
    auto& buffer = m_PrimaryCmdBuffers[idx];

    buffer.Reset();
    buffer.Begin(m_GeometryRenderPass);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_GeometryRenderPass;
    renderPassInfo.framebuffer = m_Swapchain->GetFramebuffer(idx);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_Swapchain->GetSwapchainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    buffer.BeginRenderPass(renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    buffer.ExecuteSecondaryCommandBuffer(*m_SecondaryCmdBuffers[idx].GetCommandBuffer());
    buffer.EndRenderPass();

    buffer.End();
}

void vulkanBackend::vulkanBackendImpl::createSyncObjects()
{
    imageAvailableSemaphores.resize(m_Swapchain->GetSwapchainFramebuffersCount());
    renderFinishedSemaphores.resize(m_Swapchain->GetSwapchainFramebuffersCount());
    inFlightFences.resize(m_Swapchain->GetSwapchainFramebuffersCount());

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < m_Swapchain->GetSwapchainFramebuffersCount(); i++)
    {
        if (vkCreateSemaphore(m_Device->GetLogicalDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_Device->GetLogicalDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_Device->GetLogicalDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
        {
            DBG_PRINTM("Failed to create semaphores!")
            assert(false);
        }
    }
}


//======================================================================
// VkDescriptor
void vulkanBackend::vulkanBackendImpl::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(m_Swapchain->GetSwapchainFramebuffersCount());

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    // Sum of all descriptros count from all descriptor sets
    poolSizes[1].descriptorCount = MAX_TEXTURES * m_Swapchain->GetSwapchainFramebuffersCount();

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(m_Swapchain->GetSwapchainFramebuffersCount());

    if (vkCreateDescriptorPool(m_Device->GetLogicalDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        DBG_PRINTM("failed to create descriptor pool!")
        assert(false);
    }
}
void vulkanBackend::vulkanBackendImpl::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.pImmutableSamplers = nullptr;  // Optional

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.binding = 1;
    // Max num of descriptors in this current descriptor set
    samplerLayoutBinding.descriptorCount = MAX_TEXTURES;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_Device->GetLogicalDevice(), &layoutInfo, nullptr, &m_GeometryDSL) != VK_SUCCESS)
    {
        DBG_PRINTM("failed to create descriptor set layout!!")
        assert(false);
    }
}
void vulkanBackend::vulkanBackendImpl::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(m_Swapchain->GetSwapchainFramebuffersCount(), m_GeometryDSL);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = layouts.size();
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(m_Swapchain->GetSwapchainFramebuffersCount());
    if (vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS)
    {
        DBG_PRINTM("failed to allocate descriptor sets!")
        assert(true);
    }

    for (size_t i = 0; i < descriptorSets.size(); i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_UniformBuffers[i].GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(Fleur::Graphics::SFLGeometryUBO);

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_Device->GetLogicalDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

        VkDescriptorImageInfo imageSamplerInfo{};
        imageSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageSamplerInfo.imageView = m_FallbackTexture->GetImageView();
        imageSamplerInfo.sampler = m_ImageSampler;

        std::array<VkWriteDescriptorSet, MAX_TEXTURES> descriptorImageWrites{};
        for (size_t j = 0; j < descriptorImageWrites.size(); j++)
        {
            descriptorImageWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorImageWrites[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorImageWrites[j].dstSet = descriptorSets[i];
            descriptorImageWrites[j].dstBinding = 1;
            descriptorImageWrites[j].dstArrayElement = j;
            descriptorImageWrites[j].descriptorCount = 1;
            descriptorImageWrites[j].pImageInfo = &imageSamplerInfo;
        }
        vkUpdateDescriptorSets(m_Device->GetLogicalDevice(), descriptorImageWrites.size(), descriptorImageWrites.data(), 0, nullptr);
    }
}


//======================================================================
// UniformBuffers
void vulkanBackend::vulkanBackendImpl::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(Fleur::Graphics::SFLGeometryUBO);

    m_UniformBuffers.resize(m_Swapchain->GetSwapchainFramebuffersCount());

    for (size_t i = 0; i < m_Swapchain->GetSwapchainFramebuffersCount(); i++)
    {
        m_UniformBuffers[i].Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, bufferSize, bufferSize);
    }
}
void vulkanBackend::vulkanBackendImpl::updateUniformBuffer(uint32_t currentImage, Fleur::Graphics::SFLGeometryUBO* pUbo)
{
    pUbo->proj[1][1] *= -1;
    pUbo->model = glm::mat4(1.0f);
    m_UniformBuffers[currentFrame].MemCopy(pUbo, sizeof(*pUbo));
}


//======================================================================
// VulkanMemoryAllocator
void vulkanBackend::vulkanBackendImpl::initializeVma()
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
void vulkanBackend::vulkanBackendImpl::freeVma()
{
    vmaDestroyAllocator(m_Allocator);
}


void vulkanBackend::vulkanBackendImpl::AddToDrawList(Fleur::Graphics::SFLModelView* pModelView)
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

    for (size_t i = 0; i < m_SecondaryCmdBuffers.size(); i++)
    {
        if (vkGetFenceStatus(m_Device->GetLogicalDevice(), inFlightFences[i]) == VK_SUCCESS)
        {
            UpdateGeometrySecondaryCmdBuffer(i);
            InitGeometryPrimaryCmdBuffers(i);
            continue;
        }
        m_SecondaryCmdValidation[i] = false;
    }
}
void vulkanBackend::vulkanBackendImpl::SubmitImageViews(Fleur::Graphics::SFLImageViewInfo* pInfo)
{
    for (size_t i = 0; i < pInfo->count; i++)
    {
        auto imageView = pInfo->pData + i;
        auto& gpuTexture = m_TextureMap.emplace(imageView->ID, FVkTexture()).first->second;

        uint32_t mimMapLevel = 1;
        if (imageView->layerCount == 1)
        {
            mimMapLevel = CalculateMimMapLevel(imageView->w, imageView->h);

            CreateTexture(gpuTexture, *imageView, GetVkFormat(imageView->channels), VK_IMAGE_ASPECT_COLOR_BIT, mimMapLevel);

            for (size_t i = 0; i < m_Swapchain->GetSwapchainFramebuffersCount(); i++)
            {
                if (i == currentFrame)
                {
                    UpdateDescriptorSets(descriptorSets[currentFrame], imageView->ID, gpuTexture.GetImageView(), m_ImageSampler);
                    continue;
                }

                m_DescriptorSetImageViews[i].emplace_back(imageView->ID, gpuTexture.GetImageView());
            }
        }
        else if (imageView->layerCount == CUBEMAP_LAYERS_COUNT)
        {
            VkFormat format = GetVkFormat(imageView->channels);
            uint32_t layerSize = imageView->w * imageView->h * imageView->channels;
            uint32_t imageSize = layerSize * imageView->layerCount;

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            imageInfo.extent.width = imageView->w;
            imageInfo.extent.height = imageView->h;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = imageView->layerCount;
            imageInfo.format = format;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            FVkBuffer stagingBuffer{};
            stagingBuffer.Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, imageSize, layerSize);
            stagingBuffer.MemCopy(imageView->pData, imageSize);

            VkImage cubemapImage = gpuTexture.CreateImage(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), imageInfo,
                                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

            FVkSingleTimeCommandBuffer* cmd = m_Device->GetFrameCommandBuffer();
            cmd->Begin();
            cmd->TransitionImageLayout(cubemapImage, GetVkFormat(imageView->channels), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       VK_IMAGE_ASPECT_COLOR_BIT, 1);
            cmd->CopyBufferToImage(stagingBuffer.GetBuffer(), cubemapImage, {imageView->w, imageView->h}, layerSize, imageView->layerCount);
            cmd->TransitionImageLayout(cubemapImage, GetVkFormat(imageView->channels), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1);
            cmd->Submit(m_Device->GetGraphicsQueue());
            cmd->Synchronize();
            gpuTexture.CreateImaveView();
        }
    }
}

void vulkanBackend::vulkanBackendImpl::SFLCmdBuffer::Invalidate()
{
    for (size_t i = 0; i < validation.size(); i++)
    {
        validation[i] = false;
    }
}
bool vulkanBackend::vulkanBackendImpl::SFLCmdBuffer::AreValid()
{
    for (size_t i = 0; i < validation.size(); i++)
    {
        if (validation[i] == false)
            return false;
    }
    return true;
}

VkImageView vulkanBackend::vulkanBackendImpl::createTextureImageView(VkImage& image, VkFormat format)
{
    return createImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT);
}
VkImageView vulkanBackend::vulkanBackendImpl::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
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
VkSampler vulkanBackend::vulkanBackendImpl::createTextureSampler()
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


void vulkanBackend::vulkanBackendImpl::CreateFallbackTexture(Fleur::Graphics::SFLImageView& view)
{
    VkFormat format{VK_FORMAT_R8G8B8A8_UNORM};

    m_FallbackTexture = new FVkTexture();
    CreateTexture(*m_FallbackTexture, view, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    m_FallbackTextureIdx = view.ID;
}

void vulkanBackend::vulkanBackendImpl::UpdateDescriptorSets(VkDescriptorSet& set, uint32_t idx, VkImageView imageView, VkSampler& sampler)
{
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = set;
    descriptorWrites[0].dstBinding = 1;
    descriptorWrites[0].dstArrayElement = idx;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_Device->GetLogicalDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void vulkanBackend::vulkanBackendImpl::UpdateGeometrySecondaryCmdBuffer(uint32_t idx)
{
    m_SecondaryCmdValidation[idx] = true;

    auto& buffer = m_SecondaryCmdBuffers[idx];
    buffer.Reset();
    buffer.Begin(m_GeometryRenderPass);
    buffer.BindPipeline(m_GeometryPipeline->GetPipeline());

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_Swapchain->GetSwapchainExtent().width);
    viewport.height = static_cast<float>(m_Swapchain->GetSwapchainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    buffer.SetViewport(viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_Swapchain->GetSwapchainExtent();
    buffer.SetScissors(scissor);

    buffer.BindVertexBuffer(&m_VertexBuffer->GetBuffer());
    if (m_IndexBuffer->StrideBytes() == 4)
        buffer.BindIndexBuffer(&m_IndexBuffer->GetBuffer(), VK_INDEX_TYPE_UINT32);
    else if (m_IndexBuffer->StrideBytes() == 2)
        buffer.BindIndexBuffer(&m_IndexBuffer->GetBuffer(), VK_INDEX_TYPE_UINT16);

    buffer.BindDescriptorSet(m_GeometryPipeline->GetPipelineLayout(), &descriptorSets[idx]);

    for (const auto& draw : m_DrawList)
    {
        SFLPushConstant pc{draw.material.albedo};
        buffer.PushConstant(m_GeometryPipeline->GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, pc);
        buffer.DrawIndexed(draw.indexCount, draw.indexOffset, draw.vertexOffset);
    }

    buffer.End();
}

//======================================================================
// Depth
void vulkanBackend::vulkanBackendImpl::CreateDepthBuffer(vulkanBackend::vulkanBackendImpl::Depth& depthBuffer, VkPhysicalDevice device,
                                                         VkSampleCountFlagBits samplesCount, uint32_t mimLevels)
{
    CreateDepthTexture(*depthBuffer.depthTexture, m_Swapchain->GetSwapchainExtent().width, m_Swapchain->GetSwapchainExtent().height, FindDepthFormat(device),
                       samplesCount, mimLevels);
}


void vulkanBackend::vulkanBackendImpl::update(Fleur::Graphics::SFLGeometryUBO* pUbo)
{
    if (m_WindowResizeIsInProgress)
    {
        return;
    }
    if (!m_Swapchain->ReadyToPresent())
    {
        vkDeviceWaitIdle(m_Device->GetLogicalDevice());
        m_Swapchain->Recreate(m_Surface, m_Device->GetGraphicsQueueFamilyIndex(), m_GeometryRenderPass, m_Multisampler->GetTexture()->GetImageView(),
                              m_Depth.depthTexture->GetImageView());

        for (size_t i = 0; i < m_Swapchain->GetSwapchainFramebuffersCount(); i++)
        {
            InitGeometryPrimaryCmdBuffers(i);
        }
    }
    uint32_t prevFrame = (currentFrame + m_Swapchain->GetSwapchainFramebuffersCount() - 1) % m_Swapchain->GetSwapchainFramebuffersCount();

    // Fence: CPU awaits signal from GPU here
    vkWaitForFences(m_Device->GetLogicalDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_Device->GetLogicalDevice(), 1, &inFlightFences[currentFrame]);


    if (!m_DescriptorSetImageViews[currentFrame].empty())
    {
        for (size_t i = 0; i < m_DescriptorSetImageViews[currentFrame].size(); i++)
        {
            UpdateDescriptorSets(descriptorSets[currentFrame], m_DescriptorSetImageViews[currentFrame][i].idx, m_DescriptorSetImageViews[currentFrame][i].view,
                                 m_ImageSampler);
        }
        m_DescriptorSetImageViews[currentFrame].clear();
    }

    if (!m_SecondaryCmdValidation[currentFrame])
    {
        UpdateGeometrySecondaryCmdBuffer(currentFrame);
        InitGeometryPrimaryCmdBuffers(currentFrame);
    }

    uint32_t imageIndex;
    VkResult isSwapchainValid{};
    isSwapchainValid = vkAcquireNextImageKHR(m_Device->GetLogicalDevice(), m_Swapchain->GetSwapchain(), UINT64_MAX, imageAvailableSemaphores[currentFrame],
                                             VK_NULL_HANDLE, &imageIndex);
    if (isSwapchainValid == VK_ERROR_OUT_OF_DATE_KHR || isSwapchainValid == VK_SUBOPTIMAL_KHR)
    {
        assert(false);
        std::cout << "\VK_ERROR_OUT_OF_DATE_KHR\n";
    }
    else if (isSwapchainValid != VK_SUCCESS && isSwapchainValid != VK_SUBOPTIMAL_KHR)
    {
        DBG_PRINTM("Failed to present swap chain image!")
        assert(false);
    }

    m_Skybox->Update(currentFrame, glm::mat4(1.f));

    updateUniformBuffer(currentFrame, pUbo);

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;

    submitInfo.pCommandBuffers = m_PrimaryCmdBuffers[currentFrame].GetCommandBuffer();

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
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

    currentFrame = (currentFrame + 1) % m_Swapchain->GetSwapchainFramebuffersCount();
}

void vulkanBackend::vulkanBackendImpl::SetSkybox(AssetID id)
{
    while (m_TextureMap[id].GetImageView() == nullptr)
    {
    }
    m_Skybox->SetSkybox(m_TextureMap[id].GetImageView());
    m_Skybox->Update(0, glm::mat4(1.f));
    m_Skybox->Update(1, glm::mat4(1.f));
    m_Skybox->Update(2, glm::mat4(1.f));

    m_Skybox->Record(*m_SkyboxCmd.GetCommandBuffer(), 0, m_Swapchain->GetFramebuffer(0), m_Swapchain->GetSwapchainExtent());
    m_Skybox->Record(*m_SkyboxCmd.GetCommandBuffer(), 1, m_Swapchain->GetFramebuffer(1), m_Swapchain->GetSwapchainExtent());
    m_Skybox->Record(*m_SkyboxCmd.GetCommandBuffer(), 2, m_Swapchain->GetFramebuffer(2), m_Swapchain->GetSwapchainExtent());
}

void vulkanBackend::vulkanBackendImpl::CreateTexture(FVkTexture& texture, Fleur::Graphics::SFLImageView& view, VkFormat format, VkImageAspectFlags aspect,
                                                     uint32_t mipLevels)
{
    if (mipLevels == 0)
        mipLevels = 1;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = view.w;
    imageInfo.extent.height = view.h;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (mipLevels > 1)
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    else
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkImage vkImage = texture.CreateImage(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, aspect);

    VkDeviceSize bufferImageSize = view.w * view.h * GetChannelsNumFromFormat(format);
    VkDeviceSize mapImageSize = view.w * view.h * view.channels;

    FVkBuffer stagingBuffer{};
    stagingBuffer.Init(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bufferImageSize, bufferImageSize);

    stagingBuffer.MemCopy(view.pData, static_cast<size_t>(mapImageSize));

    FVkSingleTimeCommandBuffer* frameCmd = m_Device->GetFrameCommandBuffer();
    frameCmd->Begin();
    frameCmd->TransitionImageLayout(vkImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, aspect, mipLevels);
    frameCmd->CopyBufferToImage(stagingBuffer.GetBuffer(), vkImage, {view.w, view.h}, bufferImageSize, 1);
    if (mipLevels > 1)
    {
        frameCmd->GenerateMipMaps(m_Device->GetPhysicalDevice(), vkImage, VK_FORMAT_R8G8B8A8_SRGB, view.w, view.h, mipLevels);
    }
    else
    {
        frameCmd->TransitionImageLayout(vkImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, aspect, mipLevels);
    }

    frameCmd->Submit(m_Device->GetGraphicsQueue());
    frameCmd->Synchronize();
    texture.CreateImaveView();
}

void vulkanBackend::vulkanBackendImpl::CreateDepthTexture(FVkTexture& texture, uint32_t width, uint32_t height, VkFormat format,
                                                          VkSampleCountFlagBits samplesCount, uint32_t mimLevels)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mimLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = samplesCount;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkImage vkImage = texture.CreateImage(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          GetDepthAspect(format));
    texture.CreateImaveView();

    FVkSingleTimeCommandBuffer* frameCmd = m_Device->GetFrameCommandBuffer();
    frameCmd->Begin();
    frameCmd->TransitionImageLayout(vkImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, GetDepthAspect(format),
                                    mimLevels);
    frameCmd->Submit(m_Device->GetGraphicsQueue());
    frameCmd->Synchronize();
}

void vulkanBackend::vulkanBackendImpl::StartResize()
{
    m_WindowResizeIsInProgress = true;
    std::cout << "\nStartResize\n";
}
void vulkanBackend::vulkanBackendImpl::EndResize(Fleur::SRect& rect)
{
    m_WindowResizeIsInProgress = false;
    m_Swapchain->OnWindowResized(rect);
    std::cout << "\EndResize\n";
}

void vulkanBackend::vulkanBackendImpl::CreateSkybox(AssetID id, SFLShaderInfo* pVertexShaderInfo, SFLShaderInfo* pFragmentShaderInfo)
{
    m_Skybox = new FVkSkybox();
    m_Skybox->Create(m_Device, m_Swapchain, m_FallbackTexture->GetImageView(), CreateShaderModule(pVertexShaderInfo), CreateShaderModule(pFragmentShaderInfo),
                     VK_SAMPLE_COUNT_1_BIT);
}