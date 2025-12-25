#include "Renderer_Vulkan.h"

#if defined(FLEUR_PLATFORM_WIN)
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

#include <iostream>
#include <optional>
#include <vector>

#if defined(FL_CONF_DEBUG)
#define DBG_PRINT(moduleText, text) std::cout << moduleText << text << std::endl;
#define MODULE "[Vulkan] "
#define DBG_PRINTM(text) std::cout << MODULE << text << std::endl;
#else
#define DBG_PRINT(moduleText, text)
#define MODULE
#define DBG_PRINTM(text)
#endif
//======================================================================
// Static functions
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        DBG_PRINTM("debug callback" << pCallbackData->pMessage);
        for (size_t i = 0; i < pCallbackData->objectCount; i++)
        {
            if (!pCallbackData->pObjects[i].pObjectName)
                break;
            DBG_PRINT("", "\t [Object] " << pCallbackData->pObjects[i].pObjectName);
        }
    }

    return VK_FALSE;
}
struct QueueFamilyIndices
{
    uint32_t graphicsFamily;
    uint32_t surfaceSupport;
    bool isCompleted;

    inline bool IsCompleted()
    {
        return isCompleted;
    }
};

//======================================================================
// vulkanBackend::vulkanBackendImpl
struct vulkanBackend::vulkanBackendImpl
{
    vulkanBackendImpl(void* pNativeHandle);
    ~vulkanBackendImpl();

    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkQueue presentQueue;

    QueueFamilyIndices family;
    VkDebugUtilsMessengerEXT debugMessenger;

    bool enableValidationLayers;
    std::vector<const char*> validationLayers;

    std::vector<const char*> extensions;

    // Instance
    VkInstance createInstance();
    void enableValidationLayersSupport(VkInstanceCreateInfo& createinfo);
    void enableExtensions(VkInstanceCreateInfo& createinfo);

    // Debug messages
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setupDebugMessenger();
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                          VkDebugUtilsMessengerEXT* pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

    // Physical devices
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);

    // Queue families
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    // Logical device
    void createLogicalDevice();

    // Surface
    void createSurface(void* pNativeHandle);
};

//======================================================================
// vulkanBackend
vulkanBackend::vulkanBackend(void* pNativeHandle)
    : pImpl(new vulkanBackendImpl(pNativeHandle))
{
}
vulkanBackend::~vulkanBackend()
{
    delete pImpl;
}
void vulkanBackend::Draw(DrawInfo info)
{
}


//======================================================================
// vulkanBackend::vulkanBackendImpl
vulkanBackend::vulkanBackendImpl::vulkanBackendImpl(void* pNativeHandle)
    : physicalDevice(VK_NULL_HANDLE)
{
#if defined(FL_CONF_DEBUG)
    enableValidationLayers = true;
#else
    enableValidationLayers = false;
#endif

    instance = createInstance();
    setupDebugMessenger();
    createSurface(pNativeHandle);
    pickPhysicalDevice();
    createLogicalDevice();
}
vulkanBackend::vulkanBackendImpl::~vulkanBackendImpl()
{
    if (enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

VkInstance vulkanBackend::vulkanBackendImpl::createInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Fleur Engine";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 4, 335);
    appInfo.pEngineName = "Fleur Engine";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    appInfo.apiVersion = VK_MAKE_API_VERSION(0, 0, 0, 0);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    validationLayers.emplace_back("VK_LAYER_KHRONOS_validation");

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

    extensions.emplace_back("VK_EXT_debug_utils");
    extensions.emplace_back("VK_KHR_surface");
#if defined(FLEUR_PLATFORM_WIN)
    extensions.emplace_back("VK_KHR_win32_surface");
#endif
    enableExtensions(createInfo);

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        assert(false);
    }

    return instance;
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
        DBG_PRINT("", '\t' << availableLayers[i].layerName << "spec_v: " << availableLayers[i].specVersion
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

    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
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

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
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

void vulkanBackend::vulkanBackendImpl::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        DBG_PRINTM("Failed to find GPUs with Vulkan support")
        assert(false);
    }

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    for (size_t i = 0; i < deviceCount; i++)
    {
        if (isDeviceSuitable(physicalDevices[i]))
        {
            physicalDevice = physicalDevices[i];
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        DBG_PRINTM("Failed to find a suitable GPU!")
        assert(false);
    }
}
QueueFamilyIndices vulkanBackend::vulkanBackendImpl::findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices{};
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> familyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, familyProperties.data());

    for (size_t i = 0; i < queueFamilyCount; i++)
    {
        if (familyProperties[i].queueCount > 0 && familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
            {
                indices.surfaceSupport = i;
            }
            indices.isCompleted = true;
            break;
        }
    }

    return indices;
}
bool vulkanBackend::vulkanBackendImpl::isDeviceSuitable(VkPhysicalDevice device)
{
    family = findQueueFamilies(device);

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return family.IsCompleted();
}

void vulkanBackend::vulkanBackendImpl::createLogicalDevice()
{
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = family.graphicsFamily;
    queueCreateInfo.queueCount = 1;

    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;

    createInfo.pEnabledFeatures = &deviceFeatures;

    // Queues
    std::vector<uint32_t> uniqueQueueFamilies;
    uniqueQueueFamilies.push_back(family.graphicsFamily);
    uniqueQueueFamilies.push_back(family.surfaceSupport);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(uniqueQueueFamilies.size());

    for (size_t i = 0; i < uniqueQueueFamilies.size(); i++)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = i;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos[i] = queueCreateInfo;
    }
    createInfo.queueCreateInfoCount = 2;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();


    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create logical device!");
        assert(true);
    }
    vkGetDeviceQueue(device, family.graphicsFamily, 0, &graphicsQueue);
}

void vulkanBackend::vulkanBackendImpl::createSurface(void* pNativeHandle)
{
#if defined(FLEUR_PLATFORM_WIN)
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = reinterpret_cast<HWND>(pNativeHandle);
    createInfo.hinstance = GetModuleHandle(nullptr);
#endif
    if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
    {
        DBG_PRINTM("Failed to create window surface!")
        assert(false);
    }
}
