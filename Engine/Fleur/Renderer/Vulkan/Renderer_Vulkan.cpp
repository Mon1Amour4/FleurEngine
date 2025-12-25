#include "Renderer_Vulkan.h"

#include <Windows.h>
#include <vulkan/vulkan.h>

#include <iostream>
#include <optional>

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
    std::optional<uint32_t> graphicsFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value();
    }
};

//======================================================================
// vulkanBackend::vulkanBackendImpl
struct vulkanBackend::vulkanBackendImpl
{
    vulkanBackendImpl();
    ~vulkanBackendImpl();

    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDebugUtilsMessengerEXT debugMessenger;
    bool enableValidationLayers;

    VkInstance createInstance();
    void enableValidationLayersSupport(VkInstanceCreateInfo& createinfo, const char** layernames, uint32_t layercount);
    void enableExtensions(VkInstanceCreateInfo& createinfo, const char** extensions, uint32_t count);
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
};

//======================================================================
// vulkanBackend
vulkanBackend::vulkanBackend()
    : pImpl(new vulkanBackendImpl())
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
vulkanBackend::vulkanBackendImpl::vulkanBackendImpl()
    : physicalDevice(VK_NULL_HANDLE)
{
#if defined(FL_CONF_DEBUG)
    enableValidationLayers = true;
#else
    enableValidationLayers = false;
#endif

    instance = createInstance();
    setupDebugMessenger();
    pickPhysicalDevice();
}
vulkanBackend::vulkanBackendImpl::~vulkanBackendImpl()
{
    if (enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
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

    uint32_t layersCount = 1;
    const char** layerNames = new const char*[layersCount];
    layerNames[0] = "VK_LAYER_KHRONOS_validation";

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers)
    {
        enableValidationLayersSupport(createInfo, layerNames, layersCount);

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    uint32_t fleurExtensionCount = 1;
    const char** fleurExtensions = new const char*[fleurExtensionCount];
    fleurExtensions[0] = "VK_EXT_debug_utils";
    enableExtensions(createInfo, fleurExtensions, fleurExtensionCount);

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        assert(false);
    }

    delete[] layerNames;
    delete[] fleurExtensions;

    return instance;
}
void vulkanBackend::vulkanBackendImpl::enableValidationLayersSupport(VkInstanceCreateInfo& createInfo, const char** layerNames, uint32_t layerCount)
{
    uint32_t availableLayerCount;
    vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);

    VkLayerProperties* availableLayers = new VkLayerProperties[availableLayerCount];
    vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers);
    DBG_PRINTM("Vulkan available validation layers:");
    for (size_t i = 0; i < availableLayerCount; i++)
    {
        DBG_PRINT("", '\t' << availableLayers[i].layerName << "spec_v: " << availableLayers[i].specVersion
                           << "impl_v: " << availableLayers[i].implementationVersion << ' ' << availableLayers[i].description);
    }

    createInfo.enabledLayerCount = layerCount;
    createInfo.ppEnabledLayerNames = layerNames;

    delete[] availableLayers;
}
void vulkanBackend::vulkanBackendImpl::enableExtensions(VkInstanceCreateInfo& createInfo, const char** extensions, uint32_t count)
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    VkExtensionProperties* props = new VkExtensionProperties[extensionCount];
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, props);

    DBG_PRINTM("Vulkan available extensions:");
    for (size_t i = 0; i < extensionCount; i++)
    {
        DBG_PRINT("", '\t' << props[i].extensionName << " v:" << props[i].specVersion);
    }
    delete[] props;


    createInfo.enabledExtensionCount = count;
    createInfo.ppEnabledExtensionNames = extensions;
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

    VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[deviceCount];
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices);

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

    delete[] physicalDevices;
}
QueueFamilyIndices vulkanBackend::vulkanBackendImpl::findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    VkQueueFamilyProperties* familyProperties = new VkQueueFamilyProperties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, familyProperties);

    for (size_t i = 0; i < queueFamilyCount; i++)
    {
        if (familyProperties[i].queueCount > 0 && familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
            break;
        }
    }

    delete[] familyProperties;

    return indices;
}
bool vulkanBackend::vulkanBackendImpl::isDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = findQueueFamilies(device);

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return indices.isComplete();
}
