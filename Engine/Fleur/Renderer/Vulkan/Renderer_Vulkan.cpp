#include "Renderer_Vulkan.h"

#include <Windows.h>

#include <iostream>


BOOL APIENTRY DllMain(HMODULE module, DWORD reason_for_call, LPVOID lpReserved)
{
    /* switch (reason_for_call)
     {
     case DLL_PROCESS_ATTACH:
     {
         break;
     }
     case DLL_PROCESS_DETACH:
     {
         break;
     }
     case DLL_THREAD_ATTACH:
     {
         break;
     }
     case DLL_THREAD_DETACH:
     {
         break;
     }
     }*/
    return TRUE;
}

RENDERER_BACKEND_EXPORT IRenderer* CreateRendererBackend(void)
{
    return new vulkanBackend();
}
RENDERER_BACKEND_EXPORT void DestroyRendererBackend(IRenderer* backend)
{
    delete backend;
}

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

vulkanBackend::vulkanBackend()
{
    instance = createInstance();
}
vulkanBackend::~vulkanBackend()
{
    vkDestroyInstance(instance, nullptr);
}

VkInstance vulkanBackend::createInstance()
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

void vulkanBackend::enableValidationLayersSupport(VkInstanceCreateInfo& createInfo, const char** layerNames, uint32_t layerCount)
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

void vulkanBackend::enableExtensions(VkInstanceCreateInfo& createInfo, const char** extensions, uint32_t count)
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
