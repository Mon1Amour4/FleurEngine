#include "Renderer_Vulkan.h"

#include <Windows.h>
#include <vulkan/vulkan.h>

#include <iostream>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        std::cout << "[Vulkan] debug callback" << pCallbackData->pMessage << '/n';
        for (size_t i = 0; i < pCallbackData->objectCount; i++)
        {
            std::cout << "\t [Object] " << pCallbackData->pObjects[i].pObjectName << '\n';
        }
    }

    return VK_FALSE;
}
struct vulkanBackend::vulkanBackendImpl
{
    vulkanBackendImpl();
    ~vulkanBackendImpl();
    VkInstance instance;
    VkInstance createInstance();
    void enableValidationLayersSupport(VkInstanceCreateInfo& createinfo, const char** layernames, uint32_t layercount);
    void enableExtensions(VkInstanceCreateInfo& createinfo, const char** extensions, uint32_t count);
};
vulkanBackend::vulkanBackend()
    : pImpl(new vulkanBackendImpl())
{
}
vulkanBackend::~vulkanBackend()
{
    delete pImpl;
}


vulkanBackend::vulkanBackendImpl::vulkanBackendImpl()
{
    instance = createInstance();
}
vulkanBackend::vulkanBackendImpl::~vulkanBackendImpl()
{
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
#if defined(FL_CONF_DEBUG)
    enableValidationLayersSupport(createInfo, layerNames, layersCount);
#else
    createInfo.enabledLayerCount = 0;
#endif

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
    std::cout << "Vulkan available validation layers:\n";
    for (size_t i = 0; i < availableLayerCount; i++)
    {
        std::cout << '\t' << availableLayers[i].layerName << "spec_v: " << availableLayers[i].specVersion
                  << "impl_v: " << availableLayers[i].implementationVersion << ' ' << availableLayers[i].description << '\n';
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

    std::cout << "\nVulkan available extensions:\n";
    for (size_t i = 0; i < extensionCount; i++)
    {
        std::cout << '\t' << props[i].extensionName << " v:" << props[i].specVersion << '\n';
    }
    delete[] props;


    createInfo.enabledExtensionCount = count;
    createInfo.ppEnabledExtensionNames = extensions;
}

void vulkanBackend::Draw(DrawInfo info)
{
}
