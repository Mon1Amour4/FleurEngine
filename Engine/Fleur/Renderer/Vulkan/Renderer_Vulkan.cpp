
// This entire .cpp file was so big so it was pain in the ass to navigate throughout
// I've hidden vulkanBackendImpl declaration into .hpp file

#include "PrivateVulkanImpl.hpp"


vulkanBackend::vulkanBackend(Fleur::Graphics::SFLFrame* pFrame, void* pNativeHandle, Fleur::SRect framebufferSize)
    : pImpl(new vulkanBackendImpl(pFrame, pNativeHandle, framebufferSize))
{
}
vulkanBackend::~vulkanBackend()
{
    delete pImpl;
}
void vulkanBackend::AddToDrawList(Fleur::Graphics::SFLDrawUploadInfo* pInfo)
{
    pImpl->AddToDrawList(pInfo);
}
void vulkanBackend::Update(Fleur::Graphics::SFLGeometryUBO* pUbo)
{
    pImpl->update(pUbo);
}
void vulkanBackend::ResizeEvent(Fleur::SRect& rect)
{
    pImpl->resize_event(rect);
}


//======================================================================
// vulkanBackend::vulkanBackendImpl
vulkanBackend::vulkanBackendImpl::vulkanBackendImpl(Fleur::Graphics::SFLFrame* pFrame, void* pNativeHandle, Fleur::SRect& framebufferSize)
    : m_LogicalDevice(VK_NULL_HANDLE)
{
#if defined(FL_CONF_DEBUG)
    enableValidationLayers = true;
#else
    enableValidationLayers = false;
#endif

    m_VulkanInstance = createInstance();
    setupDebugMessenger();
    createSurface(pNativeHandle);
    pickPhysicalDevice();
    createLogicalDevice();
    initializeVma();
    createSwapChain(framebufferSize);
    createImageViews();
    CreateGeometryRenderPass();
    createDescriptorSetLayout();

    CreateGeometryPipeline(pFrame->pPass->pVertexShaderInfo, pFrame->pPass->pFragmentShaderInfo, pFrame->pPass->inputAssemblyTopology);

    createFramebuffers();
    createCommandPool();

    uint32_t vertexInputDescriptorSize = 0;
    uint32_t indexInputDescriptorSize = 0;

    if (pFrame->pPass->vertexInputInfo == Fleur::Graphics::EFLVertexInputDescription::VERTEX_INPUT_VERTEX_DATA)
        vertexInputDescriptorSize = sizeof(Fleur::Graphics::SVertexData);

    if (pFrame->pPass->indexInputInfo == Fleur::Graphics::EFLIndexInputDescription::INDEX_INPUT_UINT32)
        indexInputDescriptorSize = sizeof(uint32_t);
    else if (pFrame->pPass->indexInputInfo == Fleur::Graphics::EFLIndexInputDescription::INDEX_INPUT_UINT16)
        indexInputDescriptorSize = sizeof(uint16_t);

    CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &m_VertexBuffer, 1024u * 1024ul * 512ul, vertexInputDescriptorSize);
    CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &m_IndexBuffer, 1024u * 1024ul * 256ul, indexInputDescriptorSize);

    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();

    m_GeometrySecondaryCmdBuffer = CreateCmdBuffer(VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_SECONDARY);
    createCommandBuffers();
    InitGeometryPrimaryCmdBuffers();
    createSyncObjects();
}
vulkanBackend::vulkanBackendImpl::~vulkanBackendImpl()
{
    vkDeviceWaitIdle(m_LogicalDevice);

    // Swapchain-dependent
    cleanupSwapChain();

    for (size_t i = 0; i < m_Swapchain.framebuffersCount; i++)
    {
        vkDestroyBuffer(m_LogicalDevice, uniformBuffers[i], nullptr);
        vkFreeMemory(m_LogicalDevice, uniformBuffersMemory[i], nullptr);
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

    vkDestroyCommandPool(m_LogicalDevice, commandPool, nullptr);

    vmaDestroyBuffer(m_Allocator, m_VertexBuffer.buffer, m_VertexBuffer.allocation);
    vmaDestroyBuffer(m_Allocator, m_IndexBuffer.buffer, m_IndexBuffer.allocation);
    freeVma();

    vkDestroyDevice(m_LogicalDevice, nullptr);

    if (enableValidationLayers)
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
    if (enableValidationLayers)
    {
        enableValidationLayersSupport(createInfo);

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

#if defined(FLEUR_PLATFORM_WIN)
    instanceExtensions.emplace_back("VK_KHR_win32_surface");
#endif
    enableExtensions(createInfo);

    if (vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance) != VK_SUCCESS)
    {
        assert(false);
    }

    return m_VulkanInstance;
}
void vulkanBackend::vulkanBackendImpl::enableValidationLayersSupport(VkInstanceCreateInfo& createInfo)
{
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

    createInfo.enabledLayerCount = validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();
}
void vulkanBackend::vulkanBackendImpl::enableExtensions(VkInstanceCreateInfo& createInfo)
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> props(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, props.data());

    DBG_PRINTM("Vulkan available extensions:");
    for (size_t i = 0; i < extensionCount; i++)
    {
        DBG_PRINT("", '\t' << props[i].extensionName << " v:" << props[i].specVersion);
    }

    createInfo.enabledExtensionCount = instanceExtensions.size();
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();
}
bool vulkanBackend::vulkanBackendImpl::checkDeviceExtensionSupport(VkPhysicalDevice m_LogicalDevice)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(m_LogicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableDeviceExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(m_LogicalDevice, nullptr, &extensionCount, availableDeviceExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (auto& ext : availableDeviceExtensions)
    {
        requiredExtensions.erase(ext.extensionName);
    }

    return requiredExtensions.empty();
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
            m_SPhysicalDevice.vkPhysicalDevice = physicalDevices[i];
            m_SPhysicalDevice.capabilities = querySwapChainSupport(physicalDevices[i]);
            break;
        }
    }

    if (m_SPhysicalDevice.vkPhysicalDevice == VK_NULL_HANDLE)
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

    isDeviceExtensionsSupported = checkDeviceExtensionSupport(logicalDevice);
    if (isDeviceExtensionsSupported)
    {
        SSwapchainSupport swapchainSupport = querySwapChainSupport(logicalDevice);
        isSwapchainDetailsSupported = swapchainSupport.isSwapchainSuitable();
    }
    return (isQueueFamiliesSupported && isDeviceExtensionsSupported && isSwapchainDetailsSupported);
}
vulkanBackend::vulkanBackendImpl::SUniqueFamilyQueue vulkanBackend::vulkanBackendImpl::findQueueFamilies(VkPhysicalDevice m_LogicalDevice)
{
    vulkanBackend::vulkanBackendImpl::SUniqueFamilyQueue indices{};
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
    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (vkCreateDevice(m_SPhysicalDevice.vkPhysicalDevice, &deviceCreateInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS)
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
    uint32_t imageCount = m_SPhysicalDevice.capabilities.capabilities.minImageCount + 1;
    imageCount = std::clamp(imageCount, m_SPhysicalDevice.capabilities.capabilities.minImageCount, m_SPhysicalDevice.capabilities.capabilities.maxImageCount);

    m_Swapchain.framebuffersCount = imageCount;

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(m_SPhysicalDevice.capabilities.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(m_SPhysicalDevice.capabilities.presentModes);
    VkExtent2D extent = chooseSwapExtent(m_SPhysicalDevice.capabilities.capabilities, framebufferSize);


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
    createInfo.preTransform = m_SPhysicalDevice.capabilities.capabilities.currentTransform;
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
vulkanBackend::vulkanBackendImpl::SSwapchainSupport vulkanBackend::vulkanBackendImpl::querySwapChainSupport(VkPhysicalDevice vkPhysicalDevice)
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
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_Swapchain.images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_Swapchain.imageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_LogicalDevice, &createInfo, nullptr, &m_Swapchain.imageViews[i]) != VK_SUCCESS)
        {
            DBG_PRINTM("Failed to create image views!")
            assert(false);
        }
    }
}


//======================================================================
// VkRenderPass
void vulkanBackend::vulkanBackendImpl::CreateGeometryRenderPass()
{
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

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

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

    // VkPipelineLayout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;             // Optional
    pipelineLayoutInfo.pSetLayouts = &m_GeometryDSL;   // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;     // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr;  // Optional

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
    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
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
        VkImageView attachments[] = {m_Swapchain.imageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_GeometryRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
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
    m_PrimaryCmdBuffers.resize(m_Swapchain.framebuffersCount);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_PrimaryCmdBuffers.size();

    if (vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, m_PrimaryCmdBuffers.data()) != VK_SUCCESS)
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
    for (size_t i = 0; i < m_PrimaryCmdBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                   // Optional
        beginInfo.pInheritanceInfo = nullptr;  // Optional

        if (vkBeginCommandBuffer(m_PrimaryCmdBuffers[i], &beginInfo) != VK_SUCCESS)
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

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(m_PrimaryCmdBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(m_PrimaryCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GeometryPipeline);

        VkDeviceSize offsets[] = {0};

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_Swapchain.extent.width);
        viewport.height = static_cast<float>(m_Swapchain.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(m_PrimaryCmdBuffers[i], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_Swapchain.extent;
        vkCmdSetScissor(m_PrimaryCmdBuffers[i], 0, 1, &scissor);


        vkCmdBindVertexBuffers(m_PrimaryCmdBuffers[i], 0, 1, &m_VertexBuffer.buffer, offsets);
        if (m_IndexBuffer.strideSizeBytes == 4)
            vkCmdBindIndexBuffer(m_PrimaryCmdBuffers[i], m_IndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        else if (m_IndexBuffer.strideSizeBytes == 2)
            vkCmdBindIndexBuffer(m_PrimaryCmdBuffers[i], m_IndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(m_PrimaryCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GeometryPipelineLayout, 0, 1, &descriptorSets[currentFrame], 0,
                                nullptr);

        vkCmdExecuteCommands(m_PrimaryCmdBuffers[i], 1, &m_GeometrySecondaryCmdBuffer);

        vkCmdEndRenderPass(m_PrimaryCmdBuffers[i]);

        if (vkEndCommandBuffer(m_PrimaryCmdBuffers[i]) != VK_SUCCESS)
        {
            DBG_PRINTM("Failed to record command buffer!")
            assert(false);
        }
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


void vulkanBackend::vulkanBackendImpl::update(Fleur::Graphics::SFLGeometryUBO* pUbo)
{
    if (needToUpdateSecondaryCmdBuffer)
    {
        needToUpdateSecondaryCmdBuffer = false;
        vkResetCommandBuffer(m_GeometrySecondaryCmdBuffer, 0);

        VkCommandBufferInheritanceInfo inheritanceInfo{};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.pNext = NULL;
        inheritanceInfo.renderPass = m_GeometryRenderPass;
        inheritanceInfo.subpass = 0;
        inheritanceInfo.framebuffer = m_Swapchain.framebuffers[currentFrame];


        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;  // Optional
        beginInfo.pInheritanceInfo = &inheritanceInfo;

        if (vkBeginCommandBuffer(m_GeometrySecondaryCmdBuffer, &beginInfo) != VK_SUCCESS)
        {
            DBG_PRINTM("Failed to begin recording command for secondary cmd buffer!")
            assert(false);
        }

        for (const auto& draw : m_DrawList)
        {
            vkCmdDrawIndexed(m_GeometrySecondaryCmdBuffer, draw.indexCount, 1, draw.indexOffset, draw.vertexOffset, 0);
        }

        if (vkEndCommandBuffer(m_GeometrySecondaryCmdBuffer) != VK_SUCCESS)
        {
            DBG_PRINTM("Failed to record command to secondary cmd buffer!")
            assert(false);
        }
    }

    vkWaitForFences(m_LogicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_LogicalDevice, 1, &inFlightFences[currentFrame]);

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

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_PrimaryCmdBuffers[currentFrame];

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
}

//======================================================================
// Events
void vulkanBackend::vulkanBackendImpl::resize_event(Fleur::SRect& rect)
{
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

void vulkanBackend::vulkanBackendImpl::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
                                                    VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_LogicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        DBG_PRINTM("failed to create buffer!!")
        assert(false);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_LogicalDevice, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        DBG_PRINTM("failed to allocate buffer memory!!")
        assert(false);
    }

    vkBindBufferMemory(m_LogicalDevice, buffer, bufferMemory, 0);
}


//======================================================================
// Buffer
void vulkanBackend::vulkanBackendImpl::CreateBuffer(VkBufferUsageFlags usage, SFLBuffer* pBuffer, VkDeviceSize sizeBytes, VkDeviceSize strideSize)
{
    pBuffer->sizeBytes = sizeBytes;
    pBuffer->strideSizeBytes = strideSize;

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeBytes;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &pBuffer->buffer, &pBuffer->allocation, nullptr) != VK_SUCCESS)
    {
        assert(false);
    }
}
void vulkanBackend::vulkanBackendImpl::UploadDataToBuffer(SFLBuffer* pBuffer, const void* pData, uint64_t count)
{
    uint64_t oldOffset = pBuffer->currentSizeBytes;
    pBuffer->currentSizeBytes += count * pBuffer->strideSizeBytes;

    if (vmaCopyMemoryToAllocation(m_Allocator, pData, pBuffer->allocation, oldOffset, pBuffer->strideSizeBytes * count) != VK_SUCCESS)
    {
        assert(false);
    }
}
void vulkanBackend::vulkanBackendImpl::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
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

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;  // Optional
    copyRegion.dstOffset = 0;  // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(m_LogicalDevice, commandPool, 1, &commandBuffer);
}
uint32_t vulkanBackend::vulkanBackendImpl::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_SPhysicalDevice.vkPhysicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    assert(false);
}


//======================================================================
// VkDescriptorSetLayoutBinding
void vulkanBackend::vulkanBackendImpl::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;  // Optional

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(m_LogicalDevice, &layoutInfo, nullptr, &m_GeometryDSL) != VK_SUCCESS)
    {
        DBG_PRINTM("failed to create descriptor set layout!!")
        assert(false);
    }
}

void vulkanBackend::vulkanBackendImpl::createDescriptorPool()
{
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(m_Swapchain.framebuffersCount);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(m_Swapchain.framebuffersCount);

    if (vkCreateDescriptorPool(m_LogicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        DBG_PRINTM("failed to create descriptor pool!")
        assert(false);
    }
}

void vulkanBackend::vulkanBackendImpl::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(m_Swapchain.framebuffersCount, m_GeometryDSL);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(m_Swapchain.framebuffersCount);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(m_Swapchain.framebuffersCount);
    if (vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
    {
        DBG_PRINTM("failed to allocate descriptor sets!")
        assert(true);
    }

    for (size_t i = 0; i < m_Swapchain.framebuffersCount; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(Fleur::Graphics::SFLGeometryUBO);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_LogicalDevice, 1, &descriptorWrite, 0, nullptr);
    }
}


//======================================================================
// IndexBuffer
void vulkanBackend::vulkanBackendImpl::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(Fleur::Graphics::SFLGeometryUBO);

    uniformBuffers.resize(m_Swapchain.framebuffersCount);
    uniformBuffersMemory.resize(m_Swapchain.framebuffersCount);
    uniformBuffersMapped.resize(m_Swapchain.framebuffersCount);

    for (size_t i = 0; i < m_Swapchain.framebuffersCount; i++)
    {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uniformBuffers[i], uniformBuffersMemory[i]);

        vkMapMemory(m_LogicalDevice, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void vulkanBackend::vulkanBackendImpl::updateUniformBuffer(uint32_t currentImage, Fleur::Graphics::SFLGeometryUBO* pUbo)
{
    pUbo->proj[1][1] *= -1;
    pUbo->model = glm::mat4(1.0f);
    memcpy(uniformBuffersMapped[currentImage], pUbo, sizeof(*pUbo));
}


//======================================================================
// VulkanMemoryAllocator
void vulkanBackend::vulkanBackendImpl::initializeVma()
{
    VmaAllocatorCreateInfo allocCreateInfo{};
    allocCreateInfo.instance = m_VulkanInstance;
    allocCreateInfo.physicalDevice = m_SPhysicalDevice.vkPhysicalDevice;
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

void vulkanBackend::vulkanBackendImpl::AddToDrawList(Fleur::Graphics::SFLDrawUploadInfo* pInfo)
{
    auto& draw = m_DrawList.emplace_back();

    draw.indexCount = pInfo->indexCount;
    draw.vertexOffset = m_VertexBuffer.currentSizeBytes / m_VertexBuffer.strideSizeBytes;
    draw.indexOffset = m_IndexBuffer.currentSizeBytes / m_IndexBuffer.strideSizeBytes;

    UploadDataToBuffer(&m_VertexBuffer, pInfo->pVertex, pInfo->vertexCount);
    UploadDataToBuffer(&m_IndexBuffer, pInfo->pIndex, pInfo->indexCount);

    needToUpdateSecondaryCmdBuffer = true;
}
