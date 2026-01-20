
// This entire .cpp file was so big so it was pain in the ass to navigate throughout
// I've hidden vulkanBackendImpl declaration into .hpp file

#include "PrivateVulkanImpl.hpp"
#include "VkBuffer.h"
#include "VkHelper.hpp"


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

void vulkanBackend::ResizeEvent(Fleur::SRect& rect)
{
    pImpl->resize_event(rect);
}


//======================================================================
// vulkanBackend::vulkanBackendImpl
vulkanBackend::vulkanBackendImpl::vulkanBackendImpl(bool enableValidation, Fleur::Graphics::SFLFrame& pFrame, void* pNativeHandle,
                                                    Fleur::SRect& framebufferSize, Fleur::Graphics::SFLImageView& fallback)
    : m_LogicalDevice(VK_NULL_HANDLE)
{
    m_Capabilities = new VkCapabilities(enableValidation);

    m_VulkanInstance = createInstance();
    setupDebugMessenger();
    createSurface(pNativeHandle);
    pickPhysicalDevice();
    createLogicalDevice();
    initializeVma();

    m_VertexBuffer = new FVkBuffer(m_Allocator);
    m_IndexBuffer = new FVkBuffer(m_Allocator);

    createSwapChain(framebufferSize);
    createImageViews();
    CreateGeometryRenderPass();
    createDescriptorSetLayout();

    CreateGeometryPipeline(pFrame.pPass->pVertexShaderInfo, pFrame.pPass->pFragmentShaderInfo, pFrame.pPass->inputAssemblyTopology);

    createCommandPool();
    m_Depth = CreateDepthBuffer(m_PhysicalDevice.vkPhysicalDevice);
    createFramebuffers();

    m_DescriptorSetImageViews.resize(m_Swapchain.framebuffersCount);
    m_GeometrySecondaryCmdBuffers.resize(m_Swapchain.framebuffersCount);
    m_PrimaryCmdBuffers.buffers.resize(m_Swapchain.framebuffersCount);
    m_PrimaryCmdBuffers.validation.resize(m_Swapchain.framebuffersCount);

    uint32_t vertexInputDescriptorSize = 0;
    uint32_t indexInputDescriptorSize = 0;

    if (pFrame.pPass->vertexInputInfo == Fleur::Graphics::EFLVertexInputDescription::VERTEX_INPUT_VERTEX_DATA)
        vertexInputDescriptorSize = sizeof(Fleur::Graphics::SVertexData);

    if (pFrame.pPass->indexInputInfo == Fleur::Graphics::EFLIndexInputDescription::INDEX_INPUT_UINT32)
        indexInputDescriptorSize = sizeof(uint32_t);
    else if (pFrame.pPass->indexInputInfo == Fleur::Graphics::EFLIndexInputDescription::INDEX_INPUT_UINT16)
        indexInputDescriptorSize = sizeof(uint16_t);

    m_VertexBuffer->Init(m_LogicalDevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 1024u * 1024ul * 512ul,
                         vertexInputDescriptorSize);
    m_VertexBuffer->Allocate(m_LogicalDevice, m_PhysicalDevice.vkPhysicalDevice, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    m_IndexBuffer->Init(m_LogicalDevice, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 1024u * 1024ul * 256ul, indexInputDescriptorSize);
    m_IndexBuffer->Allocate(m_LogicalDevice, m_PhysicalDevice.vkPhysicalDevice, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    createUniformBuffers();
    createDescriptorPool();

    CreateFallbackTexture(fallback);
    m_ImageSampler = createTextureSampler();

    createDescriptorSets();

    for (size_t i = 0; i < m_GeometrySecondaryCmdBuffers.size(); i++)
    {
        m_GeometrySecondaryCmdBuffers[i] = CreateCmdBuffer(VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_SECONDARY);
    }

    for (size_t i = 0; i < m_PrimaryCmdBuffers.buffers.size(); i++)
    {
        m_PrimaryCmdBuffers.buffers[i] = CreateCmdBuffer(VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    }
    for (size_t i = 0; i < m_GeometrySecondaryCmdBuffers.size(); i++)
    {
        UpdateGeometrySecondaryCmdBuffer(i);
    }
    InitGeometryPrimaryCmdBuffers();
    createSyncObjects();
}
vulkanBackend::vulkanBackendImpl::~vulkanBackendImpl()
{
    delete m_Capabilities;
    delete m_VertexBuffer;
    delete m_IndexBuffer;

    vkDeviceWaitIdle(m_LogicalDevice);

    // Swapchain-dependent
    cleanupSwapChain();

    for (size_t i = 0; i < m_Swapchain.framebuffersCount; i++)
    {
        // TODO
        /*vkDestroyBuffer(m_LogicalDevice, uniformBuffers[i], nullptr);
        vkFreeMemory(m_LogicalDevice, uniformBuffersMemory[i], nullptr);*/
    }

    vkDestroyDescriptorPool(m_LogicalDevice, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_LogicalDevice, m_GeometryDSL, nullptr);

    vkDestroyPipeline(m_LogicalDevice, m_GeometryPipeline, nullptr);
    vkDestroyPipelineLayout(m_LogicalDevice, m_GeometryPipelineLayout, nullptr);

    vkDestroyRenderPass(m_LogicalDevice, m_GeometryRenderPass, nullptr);
    // Sync
    for (size_t i = 0; i < m_Swapchain.framebuffersCount; i++)
    {
        vkDestroySemaphore(m_LogicalDevice, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_LogicalDevice, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_LogicalDevice, inFlightFences[i], nullptr);
    }

    vkDestroySampler(m_LogicalDevice, m_ImageSampler, nullptr);

    vkDestroyCommandPool(m_LogicalDevice, commandPool, nullptr);

    vmaDestroyBuffer(m_Allocator, m_VertexBuffer->Buffer(), m_VertexBuffer->Allocation());
    vmaDestroyBuffer(m_Allocator, m_IndexBuffer->Buffer(), m_IndexBuffer->Allocation());
    freeVma();

    vkDestroyDevice(m_LogicalDevice, nullptr);

    if (m_Capabilities->ValidationEnabled())
        DestroyDebugUtilsMessengerEXT(m_VulkanInstance, debugMessenger, nullptr);

    vkDestroySurfaceKHR(m_VulkanInstance, surface, nullptr);
    vkDestroyInstance(m_VulkanInstance, nullptr);
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
// VkSurfaceKHR
void vulkanBackend::vulkanBackendImpl::createSurface(void* pNativeHandle)
{
#if defined(FLEUR_PLATFORM_WIN)
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = reinterpret_cast<HWND>(pNativeHandle);
    createInfo.hinstance = GetModuleHandle(nullptr);

    if (vkCreateWin32SurfaceKHR(m_VulkanInstance, &createInfo, nullptr, &surface) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create window surface!")
        assert(false);
    }
#endif
}


//======================================================================
// VkPhysicalDevice
void vulkanBackend::vulkanBackendImpl::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        DBG_PRINTM("Failed to find GPUs with Vulkan support")
        assert(false);
    }

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, physicalDevices.data());

    // Sort devices so we can bring GPU\dGPU to the first place
    std::vector<VkPhysicalDevice> nonDiscreateGPU;
    std::vector<VkPhysicalDevice> discreateGPU;
    for (size_t i = 0; i < deviceCount; i++)
    {
        VkPhysicalDeviceProperties deviceProperties{};
        vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
        if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            nonDiscreateGPU.push_back(physicalDevices[i]);
        else
            discreateGPU.push_back(physicalDevices[i]);
    }
    physicalDevices.clear();
    physicalDevices.assign(discreateGPU.begin(), discreateGPU.end());
    physicalDevices.insert(physicalDevices.end(), nonDiscreateGPU.begin(), nonDiscreateGPU.end());

    for (size_t i = 0; i < deviceCount; i++)
    {
        if (isDeviceSuitable(physicalDevices[i]))
        {
            m_PhysicalDevice.vkPhysicalDevice = physicalDevices[i];
            m_PhysicalDevice.capabilities = querySwapChainSupport(physicalDevices[i]);
            break;
        }
    }

    if (m_PhysicalDevice.vkPhysicalDevice == VK_NULL_HANDLE)
    {
        DBG_PRINTM("Failed to find a suitable GPU!")
        assert(false);
    }
}
bool vulkanBackend::vulkanBackendImpl::isDeviceSuitable(VkPhysicalDevice& logicalDevice)
{
    bool isQueueFamiliesSupported = false;
    bool isDeviceExtensionsSupported = false;
    bool isSwapchainDetailsSupported = false;

    m_GraphicsQueueFamily = findQueueFamilies(logicalDevice);
    isQueueFamiliesSupported = m_GraphicsQueueFamily.is_valid();

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(logicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(logicalDevice, &deviceFeatures);

    isDeviceExtensionsSupported = m_Capabilities->CheckDeviceExtensionSupport(logicalDevice);
    if (isDeviceExtensionsSupported)
    {
        SSwapchainSupport swapchainSupport = querySwapChainSupport(logicalDevice);
        isSwapchainDetailsSupported = swapchainSupport.isSwapchainSuitable();
    }
    return (isQueueFamiliesSupported && isDeviceExtensionsSupported && isSwapchainDetailsSupported);
}
SUniqueFamilyQueue vulkanBackend::vulkanBackendImpl::findQueueFamilies(VkPhysicalDevice m_LogicalDevice)
{
    SUniqueFamilyQueue indices{};
    uint32_t queueFamilyCount = 0;

    // Device features 2
    // VkPhysicalDeviceFeatures2 deviceFeatures{};
    // deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    // vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);

    vkGetPhysicalDeviceQueueFamilyProperties(m_LogicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> familyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_LogicalDevice, &queueFamilyCount, familyProperties.data());

    for (size_t i = 0; i < queueFamilyCount; i++)
    {
        if (familyProperties[i].queueCount > 0 && familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.familyIndex = i;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_LogicalDevice, i, surface, &presentSupport);
            if (presentSupport)
            {
                indices.swapchainSupport = true;
            }
            indices.availableQueueCount = familyProperties[i].queueCount;
            break;
        }
    }

    return indices;
}
void vulkanBackend::vulkanBackendImpl::createLogicalDevice()
{
    // Creation of one QueueFamily
    std::array<float, 2> queuePriority{1.0f, 1.0f};
    uint32_t queueCount = 2;
    VkDeviceQueueCreateInfo uniqueFamilyQueueCreateInfo{};
    uniqueFamilyQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    uniqueFamilyQueueCreateInfo.queueFamilyIndex = m_GraphicsQueueFamily.familyIndex;
    uniqueFamilyQueueCreateInfo.queueCount = 2;  // Queues count in this Family
    uniqueFamilyQueueCreateInfo.pQueuePriorities = queuePriority.data();

    VkPhysicalDeviceFeatures deviceFeatures{};  // Empty for now

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &uniqueFamilyQueueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;            // Unique QueueFamilies count
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;  // Pointer to array of Unique QueueFamilies
    deviceCreateInfo.ppEnabledExtensionNames = m_Capabilities->DeviceExtensionsData();
    deviceCreateInfo.enabledExtensionCount = m_Capabilities->DeviceExtensionsCount();

    if (vkCreateDevice(m_PhysicalDevice.vkPhysicalDevice, &deviceCreateInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create logical device!");
        assert(true);
    }
    vkGetDeviceQueue(m_LogicalDevice, m_GraphicsQueueFamily.familyIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(m_LogicalDevice, m_GraphicsQueueFamily.familyIndex, 1, &presentQueue);
}


//======================================================================
// VkSwapchainKHR
void vulkanBackend::vulkanBackendImpl::createSwapChain(Fleur::SRect& framebufferSize)
{
    uint32_t imageCount = m_PhysicalDevice.capabilities.capabilities.minImageCount + 1;
    imageCount = std::clamp(imageCount, m_PhysicalDevice.capabilities.capabilities.minImageCount, m_PhysicalDevice.capabilities.capabilities.maxImageCount);

    m_Swapchain.framebuffersCount = imageCount;

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(m_PhysicalDevice.capabilities.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(m_PhysicalDevice.capabilities.presentModes);
    VkExtent2D extent = chooseSwapExtent(m_PhysicalDevice.capabilities.capabilities, framebufferSize);


    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {m_GraphicsQueueFamily.familyIndex};

    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
    createInfo.preTransform = m_PhysicalDevice.capabilities.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_LogicalDevice, &createInfo, nullptr, &m_Swapchain.swapchain) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create swap chain!!")
        assert(false);
    }

    uint32_t realSwapchainImagesCount = 0;
    vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain.swapchain, &realSwapchainImagesCount, nullptr);

    assert(realSwapchainImagesCount == imageCount);

    m_Swapchain.images.resize(realSwapchainImagesCount);
    m_Swapchain.imageViews.resize(realSwapchainImagesCount);
    vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain.swapchain, &realSwapchainImagesCount, m_Swapchain.images.data());

    m_Swapchain.imageFormat = surfaceFormat.format;
    m_Swapchain.extent = extent;
}
SSwapchainSupport vulkanBackend::vulkanBackendImpl::querySwapChainSupport(VkPhysicalDevice vkPhysicalDevice)
{
    SSwapchainSupport details;

    // Basic surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, surface, &details.capabilities);

    // Supported surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, surface, &formatCount, details.formats.data());
    }

    // Supported present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, surface, &presentModeCount, details.presentModes.data());
    }


    return details;
}
VkSurfaceFormatKHR vulkanBackend::vulkanBackendImpl::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}
VkPresentModeKHR vulkanBackend::vulkanBackendImpl::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D vulkanBackend::vulkanBackendImpl::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Fleur::SRect& framebufferSize)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = {framebufferSize.width, framebufferSize.height};
        std::cout << "\nFramebuffer size: " << framebufferSize.width << " , " << framebufferSize.height << "\n";
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}


//======================================================================
// VkImageView
void vulkanBackend::vulkanBackendImpl::createImageViews()
{
    m_Swapchain.imageViews.resize(m_Swapchain.imageViews.size());

    for (size_t i = 0; i < m_Swapchain.imageViews.size(); i++)
    {
        m_Swapchain.imageViews[i] = createImageView(m_Swapchain.images[i], m_Swapchain.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}


//======================================================================
// VkRenderPass
void vulkanBackend::vulkanBackendImpl::CreateGeometryRenderPass()
{
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = FindDepthFormat(m_PhysicalDevice.vkPhysicalDevice);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
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
    colorAttachment.format = m_Swapchain.imageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;


    if (vkCreateRenderPass(m_LogicalDevice, &renderPassInfo, nullptr, &m_GeometryRenderPass) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create render pass!")
        assert(false);
    }
}


//======================================================================
// VkPipeline
void vulkanBackend::vulkanBackendImpl::CreateGeometryPipeline(Fleur::Graphics::SFLShaderInfo* pVertexInfo, Fleur::Graphics::SFLShaderInfo* pFragmentInfo,
                                                              Fleur::Graphics::EFLInputAssemblyTopology pInputAssemblyTopology)
{
    VkShaderModule vertShaderModule = CreateShaderModule(pVertexInfo);
    VkShaderModule fragShaderModule = CreateShaderModule(pFragmentInfo);

    // Shaders
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // PushConstant
    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(uint32_t);
    pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // VkPipelineLayout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;            // Optional
    pipelineLayoutInfo.pSetLayouts = &m_GeometryDSL;  // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    if (vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_GeometryPipelineLayout) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create pipeline layout!")
        assert(false);
    }

    auto bindingDescription = GetVertexDataBindingDescriptor();
    auto attributeDescriptions = GetVertexDataAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    if (pInputAssemblyTopology == Fleur::Graphics::FL_INPUT_ASSEMBLY_TOPOLOGY_TRIANGLE_LIST)
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_Swapchain.extent.width;
    viewport.height = (float)m_Swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_Swapchain.extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;           // Optional
    multisampling.pSampleMask = nullptr;             // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
    multisampling.alphaToOneEnable = VK_FALSE;       // Optional

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
    rasterizer.depthBiasClamp = 0.0f;           // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f;     // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;  // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;  // Optional
    colorBlending.blendConstants[1] = 0.0f;  // Optional
    colorBlending.blendConstants[2] = 0.0f;  // Optional
    colorBlending.blendConstants[3] = 0.0f;  // Optional

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;  // Optional
    depthStencil.maxDepthBounds = 1.0f;  // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};  // Optional
    depthStencil.back = {};   // Optional

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;  // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = m_GeometryPipelineLayout;
    pipelineInfo.renderPass = m_GeometryRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
    pipelineInfo.basePipelineIndex = -1;               // Optional

    if (vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GeometryPipeline) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create graphics pipeline!")
        assert(false);
    }

    vkDestroyShaderModule(m_LogicalDevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_LogicalDevice, vertShaderModule, nullptr);
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
    if (vkCreateShaderModule(m_LogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create shader module!")
        assert(false);
    }

    return shaderModule;
}


//======================================================================
// VkFramebuffer
void vulkanBackend::vulkanBackendImpl::createFramebuffers()
{
    m_Swapchain.framebuffers.resize(m_Swapchain.imageViews.size());

    for (size_t i = 0; i < m_Swapchain.imageViews.size(); i++)
    {
        std::array<VkImageView, 2> attachments = {m_Swapchain.imageViews[i], m_Depth.depthImageView};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_GeometryRenderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_Swapchain.extent.width;
        framebufferInfo.height = m_Swapchain.extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_LogicalDevice, &framebufferInfo, nullptr, &m_Swapchain.framebuffers[i]) != VK_SUCCESS)
        {
            DBG_PRINTM("Failed to create graphics framebuffer!")
            assert(false);
        }
    }
}


//======================================================================
// VkCommandPool
void vulkanBackend::vulkanBackendImpl::createCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_GraphicsQueueFamily.familyIndex;

    if (vkCreateCommandPool(m_LogicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create command pool!")
        assert(false);
    }
}


//======================================================================
// VkCommandBuffer
void vulkanBackend::vulkanBackendImpl::createCommandBuffers()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_PrimaryCmdBuffers.buffers.size();

    if (vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, m_PrimaryCmdBuffers.buffers.data()) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create command buffers!")
        assert(false);
    }
}
VkCommandBuffer vulkanBackend::vulkanBackendImpl::CreateCmdBuffer(VkCommandBufferLevel level)
{
    VkCommandBuffer cmdBuffer{};

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &cmdBuffer) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create command buffer!")
        assert(false);
    }

    return cmdBuffer;
}
void vulkanBackend::vulkanBackendImpl::InitGeometryPrimaryCmdBuffers()
{
    for (size_t i = 0; i < m_PrimaryCmdBuffers.buffers.size(); i++)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                   // Optional
        beginInfo.pInheritanceInfo = nullptr;  // Optional

        if (vkBeginCommandBuffer(m_PrimaryCmdBuffers.buffers[i], &beginInfo) != VK_SUCCESS)
        {
            DBG_PRINTM("Failed to begin recording command buffer!")
            assert(false);
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_GeometryRenderPass;
        renderPassInfo.framebuffer = m_Swapchain.framebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_Swapchain.extent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(m_PrimaryCmdBuffers.buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        vkCmdExecuteCommands(m_PrimaryCmdBuffers.buffers[i], 1, &m_GeometrySecondaryCmdBuffers[i]);
        vkCmdEndRenderPass(m_PrimaryCmdBuffers.buffers[i]);

        if (vkEndCommandBuffer(m_PrimaryCmdBuffers.buffers[i]) != VK_SUCCESS)
        {
            DBG_PRINTM("Failed to record command buffer!")
            assert(false);
        }
        m_PrimaryCmdBuffers.validation[i] = true;
    }
}

void vulkanBackend::vulkanBackendImpl::createSyncObjects()
{
    imageAvailableSemaphores.resize(m_Swapchain.framebuffersCount);
    renderFinishedSemaphores.resize(m_Swapchain.framebuffersCount);
    inFlightFences.resize(m_Swapchain.framebuffersCount);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < m_Swapchain.framebuffersCount; i++)
    {
        if (vkCreateSemaphore(m_LogicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_LogicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_LogicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
        {
            DBG_PRINTM("Failed to create semaphores!")
            assert(false);
        }
    }
}


//======================================================================
// Events
void vulkanBackend::vulkanBackendImpl::resize_event(Fleur::SRect& rect)
{
    static bool first = true;
    if (first)
    {
        first = false;
        return;
    }
    framebufferResized = true;
    surfaceRect = rect;
}


//======================================================================
// Cleanup
void vulkanBackend::vulkanBackendImpl::cleanupSwapChain()
{
    for (auto framebuffer : m_Swapchain.framebuffers)
    {
        vkDestroyFramebuffer(m_LogicalDevice, framebuffer, nullptr);
    }

    for (auto imageView : m_Swapchain.imageViews)
    {
        vkDestroyImageView(m_LogicalDevice, imageView, nullptr);
    }

    vkDestroySwapchainKHR(m_LogicalDevice, m_Swapchain.swapchain, nullptr);
}
void vulkanBackend::vulkanBackendImpl::recreateSwapChain()
{
    vkDeviceWaitIdle(m_LogicalDevice);

    cleanupSwapChain();

    createSwapChain(surfaceRect);
    createImageViews();
    createFramebuffers();
}


//======================================================================
// VkDescriptor
void vulkanBackend::vulkanBackendImpl::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(m_Swapchain.framebuffersCount);

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    // Sum of all descriptros count from all descriptor sets
    poolSizes[1].descriptorCount = MAX_TEXTURES * m_Swapchain.framebuffersCount;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(m_Swapchain.framebuffersCount);

    if (vkCreateDescriptorPool(m_LogicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
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

    if (vkCreateDescriptorSetLayout(m_LogicalDevice, &layoutInfo, nullptr, &m_GeometryDSL) != VK_SUCCESS)
    {
        DBG_PRINTM("failed to create descriptor set layout!!")
        assert(false);
    }
}
void vulkanBackend::vulkanBackendImpl::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(m_Swapchain.framebuffersCount, m_GeometryDSL);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = layouts.size();
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(m_Swapchain.framebuffersCount);
    if (vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
    {
        DBG_PRINTM("failed to allocate descriptor sets!")
        assert(true);
    }

    for (size_t i = 0; i < descriptorSets.size(); i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i].Buffer();
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

        vkUpdateDescriptorSets(m_LogicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

        auto& fallback = m_TextureMap[m_FallbackTextureIdx];
        VkDescriptorImageInfo imageSamplerInfo{};
        imageSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageSamplerInfo.imageView = fallback.view;
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
        vkUpdateDescriptorSets(m_LogicalDevice, descriptorImageWrites.size(), descriptorImageWrites.data(), 0, nullptr);
    }
}


//======================================================================
// UniformBuffers
void vulkanBackend::vulkanBackendImpl::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(Fleur::Graphics::SFLGeometryUBO);

    uniformBuffers.resize(m_Swapchain.framebuffersCount);

    for (size_t i = 0; i < m_Swapchain.framebuffersCount; i++)
    {
        uniformBuffers[i] = FVkBuffer(m_Allocator);
        uniformBuffers[i].Init(m_LogicalDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, bufferSize, bufferSize);
        uniformBuffers[i].Allocate(m_LogicalDevice, m_PhysicalDevice.vkPhysicalDevice,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        uniformBuffers[i].Map(m_LogicalDevice);
    }
}
void vulkanBackend::vulkanBackendImpl::updateUniformBuffer(uint32_t currentImage, Fleur::Graphics::SFLGeometryUBO* pUbo)
{
    pUbo->proj[1][1] *= -1;
    pUbo->model = glm::mat4(1.0f);
    memcpy(uniformBuffers[currentFrame].MappedMemory(), pUbo, sizeof(*pUbo));
}


//======================================================================
// VulkanMemoryAllocator
void vulkanBackend::vulkanBackendImpl::initializeVma()
{
    VmaAllocatorCreateInfo allocCreateInfo{};
    allocCreateInfo.instance = m_VulkanInstance;
    allocCreateInfo.physicalDevice = m_PhysicalDevice.vkPhysicalDevice;
    allocCreateInfo.device = m_LogicalDevice;
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


//======================================================================
// VertexDescriptors
VkVertexInputBindingDescription vulkanBackend::vulkanBackendImpl::GetVertexDataBindingDescriptor()
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Fleur::Graphics::SVertexData);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}
std::array<VkVertexInputAttributeDescription, 3> vulkanBackend::vulkanBackendImpl::GetVertexDataAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Fleur::Graphics::SVertexData, Position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Fleur::Graphics::SVertexData, TexCoord);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Fleur::Graphics::SVertexData, Normal);

    return attributeDescriptions;
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

    needToUpdateSecondaryCmdBuffer = true;
    m_PrimaryCmdBuffers.Invalidate();
}
void vulkanBackend::vulkanBackendImpl::SubmitImageViews(Fleur::Graphics::SFLImageViewInfo* pInfo)
{
    for (size_t i = 0; i < pInfo->count; i++)
    {
        auto imageView = pInfo->pData + i;
        auto& gpuTexture = m_TextureMap.emplace(imageView->ID, SGPUTexture()).first->second;

        VkFormat format{};
        switch (imageView->channels)
        {
        case 1:
            format = VK_FORMAT_R8_UNORM;
            break;
        case 3:
            format = VK_FORMAT_R8G8B8A8_SRGB;
            break;
        case 4:
            format = VK_FORMAT_R8G8B8A8_SRGB;
            break;
        }

        CreateTextureImage(*imageView, gpuTexture.image, gpuTexture.memory, format);
        gpuTexture.view = createTextureImageView(gpuTexture.image, format);

        for (size_t i = 0; i < m_Swapchain.framebuffersCount; i++)
        {
            if (i == currentFrame)
            {
                UpdateDescriptorSets(descriptorSets[currentFrame], imageView->ID, gpuTexture.view, m_ImageSampler);
                continue;
            }

            m_DescriptorSetImageViews[i].emplace_back(imageView->ID, gpuTexture.view);
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


VkCommandBuffer vulkanBackend::vulkanBackendImpl::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}
void vulkanBackend::vulkanBackendImpl::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(m_LogicalDevice, commandPool, 1, &commandBuffer);
}
void vulkanBackend::vulkanBackendImpl::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                                                             VkImageAspectFlags aspectMask)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        assert(false);
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(commandBuffer);
}
void vulkanBackend::vulkanBackendImpl::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}
void vulkanBackend::vulkanBackendImpl::CreateTextureImage(Fleur::Graphics::SFLImageView& imageView, VkImage& image, VkDeviceMemory& imageMemory,
                                                          VkFormat format)
{
    // VkDeviceMemory stagingBufferMemory;
    VkDeviceSize bufferImageSize = imageView.w * imageView.h * GetChannelsNumFromFormat(format);
    VkDeviceSize mapImageSize = imageView.w * imageView.h * imageView.channels;

    FVkBuffer stagingBuffer = FVkBuffer(m_Allocator);
    stagingBuffer.Init(m_LogicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bufferImageSize, bufferImageSize);
    stagingBuffer.Allocate(m_LogicalDevice, m_PhysicalDevice.vkPhysicalDevice, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data = stagingBuffer.Map(m_LogicalDevice);
    memcpy(data, imageView.pData, static_cast<size_t>(mapImageSize));
    stagingBuffer.Unmap(m_LogicalDevice);

    createImage(imageView.w, imageView.h, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

    transitionImageLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

    copyBufferToImage(stagingBuffer.Buffer(), image, static_cast<uint32_t>(imageView.w), static_cast<uint32_t>(imageView.h));

    transitionImageLayout(image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

    vkDestroyBuffer(m_LogicalDevice, stagingBuffer.Buffer(), nullptr);
    vkFreeMemory(m_LogicalDevice, stagingBuffer.Memory(), nullptr);
}

void vulkanBackend::vulkanBackendImpl::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                                                   VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_LogicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create image!");
        assert(true);
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_LogicalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(m_PhysicalDevice.vkPhysicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to Allocate memory for image!");
        assert(true);
    }

    vkBindImageMemory(m_LogicalDevice, image, imageMemory, 0);
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
    if (vkCreateImageView(m_LogicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
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
    vkGetPhysicalDeviceProperties(m_PhysicalDevice.vkPhysicalDevice, &properties);

    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkSampler sampler{};
    if (vkCreateSampler(m_LogicalDevice, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create texture sampler!");
        assert(true);
    }

    return sampler;
}

uint32_t vulkanBackend::vulkanBackendImpl::GetChannelsNumFromFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_R8G8B8A8_UNORM:
        return 4;
    case VK_FORMAT_R8_UNORM:
        return 1;
    default:
        assert(false);
        break;
    }
}

void vulkanBackend::vulkanBackendImpl::CreateFallbackTexture(Fleur::Graphics::SFLImageView& view)
{
    VkFormat format{VK_FORMAT_R8G8B8A8_UNORM};

    auto& gpuTexture = m_TextureMap.emplace(view.ID, SGPUTexture()).first->second;
    CreateTextureImage(view, gpuTexture.image, gpuTexture.memory, format);
    gpuTexture.view = createTextureImageView(gpuTexture.image, format);
    CreateTextureImage(view, m_FallbackTexture.image, m_FallbackTexture.memory, format);
    m_FallbackTexture.view = createTextureImageView(m_FallbackTexture.image, format);
    m_FallbackTextureIdx = view.ID;
}

void vulkanBackend::vulkanBackendImpl::UpdateDescriptorSets(VkDescriptorSet& set, uint32_t idx, VkImageView& imageView, VkSampler& sampler)
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

    vkUpdateDescriptorSets(m_LogicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void vulkanBackend::vulkanBackendImpl::UpdateGeometrySecondaryCmdBuffer(uint32_t idx)
{
    vkResetCommandBuffer(m_GeometrySecondaryCmdBuffers[idx], 0);

    VkCommandBufferInheritanceInfo inheritanceInfo{};
    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.pNext = NULL;
    inheritanceInfo.renderPass = m_GeometryRenderPass;
    inheritanceInfo.subpass = 0;
    inheritanceInfo.framebuffer = VK_NULL_HANDLE;


    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    beginInfo.pInheritanceInfo = &inheritanceInfo;

    if (vkBeginCommandBuffer(m_GeometrySecondaryCmdBuffers[idx], &beginInfo) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to begin recording command for secondary cmd buffer!")
        assert(false);
    }
    vkCmdBindPipeline(m_GeometrySecondaryCmdBuffers[idx], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GeometryPipeline);

    VkDeviceSize offsets[] = {0};

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_Swapchain.extent.width);
    viewport.height = static_cast<float>(m_Swapchain.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_GeometrySecondaryCmdBuffers[idx], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_Swapchain.extent;
    vkCmdSetScissor(m_GeometrySecondaryCmdBuffers[idx], 0, 1, &scissor);


    vkCmdBindVertexBuffers(m_GeometrySecondaryCmdBuffers[idx], 0, 1, &m_VertexBuffer->Buffer(), offsets);
    if (m_IndexBuffer->StrideBytes() == 4)
        vkCmdBindIndexBuffer(m_GeometrySecondaryCmdBuffers[idx], m_IndexBuffer->Buffer(), 0, VK_INDEX_TYPE_UINT32);
    else if (m_IndexBuffer->StrideBytes() == 2)
        vkCmdBindIndexBuffer(m_GeometrySecondaryCmdBuffers[idx], m_IndexBuffer->Buffer(), 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(m_GeometrySecondaryCmdBuffers[idx], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GeometryPipelineLayout, 0, 1, &descriptorSets[currentFrame],
                            0, nullptr);
    struct SFLPushConstant
    {
        uint32_t albedoIdx;
    };

    for (const auto& draw : m_DrawList)
    {
        SFLPushConstant pc{draw.material.albedo};
        if (pc.albedoIdx == 3)
            __debugbreak();

        vkCmdPushConstants(m_GeometrySecondaryCmdBuffers[idx], m_GeometryPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SFLPushConstant), &pc);
        vkCmdDrawIndexed(m_GeometrySecondaryCmdBuffers[idx], draw.indexCount, 1, draw.indexOffset, draw.vertexOffset, 0);
    }

    if (vkEndCommandBuffer(m_GeometrySecondaryCmdBuffers[idx]) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to record command to secondary cmd buffer!")
        assert(false);
    }
}
void vulkanBackend::vulkanBackendImpl::UpdateGeometryPrimaryBuffer(uint32_t bufferIdx)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                   // Optional
    beginInfo.pInheritanceInfo = nullptr;  // Optional

    if (vkBeginCommandBuffer(m_PrimaryCmdBuffers.buffers[bufferIdx], &beginInfo) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to begin recording command buffer!")
        assert(false);
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_GeometryRenderPass;
    renderPassInfo.framebuffer = m_Swapchain.framebuffers[bufferIdx];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_Swapchain.extent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(m_PrimaryCmdBuffers.buffers[bufferIdx], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    vkCmdExecuteCommands(m_PrimaryCmdBuffers.buffers[bufferIdx], 1, &m_GeometrySecondaryCmdBuffers[bufferIdx]);
    vkCmdEndRenderPass(m_PrimaryCmdBuffers.buffers[bufferIdx]);

    if (vkEndCommandBuffer(m_PrimaryCmdBuffers.buffers[bufferIdx]) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to record command buffer!")
        assert(false);
    }
}


//======================================================================
// Depth
vulkanBackend::vulkanBackendImpl::Depth vulkanBackend::vulkanBackendImpl::CreateDepthBuffer(VkPhysicalDevice device)
{
    Depth depth{};
    VkFormat depthFormat = FindDepthFormat(device);
    createImage(m_Swapchain.extent.width, m_Swapchain.extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth.depthImage, depth.depthImageMemory);
    depth.depthImageView = createImageView(depth.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    transitionImageLayout(depth.depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_ASPECT_DEPTH_BIT);

    return depth;
}

VkFormat vulkanBackend::vulkanBackendImpl::FindDepthFormat(VkPhysicalDevice device)
{
    VkFormat format = FindSupportedFormat(device, {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL,
                                          VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    /*if (!HasStencilComponent(format))
        assert(false);*/
    return format;
}
VkFormat vulkanBackend::vulkanBackendImpl::FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                                               VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }

        assert(false);
    }
}
bool vulkanBackend::vulkanBackendImpl::HasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}


void vulkanBackend::vulkanBackendImpl::update(Fleur::Graphics::SFLGeometryUBO* pUbo)
{
    // Fence: CPU awaits signal from GPU here
    vkWaitForFences(m_LogicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_LogicalDevice, 1, &inFlightFences[currentFrame]);

    if (!m_DescriptorSetImageViews[currentFrame].empty())
    {
        for (size_t i = 0; i < m_DescriptorSetImageViews[currentFrame].size(); i++)
        {
            UpdateDescriptorSets(descriptorSets[currentFrame], m_DescriptorSetImageViews[currentFrame][i].idx, m_DescriptorSetImageViews[currentFrame][i].view,
                                 m_ImageSampler);
        }
        m_DescriptorSetImageViews[currentFrame].clear();
    }
    if (needToUpdateSecondaryCmdBuffer)
    {
        uint32_t prevFrame = (currentFrame + m_Swapchain.framebuffersCount - 1) % m_Swapchain.framebuffersCount;
        vkWaitForFences(m_LogicalDevice, 1, &inFlightFences[prevFrame], VK_TRUE, UINT64_MAX);
        UpdateGeometrySecondaryCmdBuffer(currentFrame);
        if (!m_PrimaryCmdBuffers.validation[currentFrame])
            UpdateGeometryPrimaryBuffer(currentFrame);

        if (m_PrimaryCmdBuffers.AreValid())
            needToUpdateSecondaryCmdBuffer = false;
    }


    uint32_t imageIndex;
    VkResult isSwapchainValid{};
    isSwapchainValid =
        vkAcquireNextImageKHR(m_LogicalDevice, m_Swapchain.swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (isSwapchainValid == VK_ERROR_OUT_OF_DATE_KHR || isSwapchainValid == VK_SUBOPTIMAL_KHR || framebufferResized)
    {
        framebufferResized = false;
        recreateSwapChain();
    }
    else if (isSwapchainValid != VK_SUCCESS && isSwapchainValid != VK_SUBOPTIMAL_KHR)
    {
        DBG_PRINTM("Failed to present swap chain image!")
        assert(false);
    }
    vkResetFences(m_LogicalDevice, 1, &inFlightFences[currentFrame]);

    updateUniformBuffer(currentFrame, pUbo);

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_PrimaryCmdBuffers.buffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to submit draw command buffer!")
        assert(false);
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.pResults = nullptr;  // Optional

    VkSwapchainKHR swapChains[] = {m_Swapchain.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(presentQueue, &presentInfo);

    currentFrame = (currentFrame + 1) % m_Swapchain.framebuffersCount;
    uint32_t prev = (currentFrame - 1) % m_Swapchain.framebuffersCount;
}
