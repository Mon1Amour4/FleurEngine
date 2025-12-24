#include <Windows.h>
#include <iostream>
#include "Renderer_Vulkan.h"

BOOL APIENTRY DllMain(HMODULE module, DWORD reason_for_call, LPVOID lpReserved)
{
    switch (reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        std::cout << "Renderer_Vulkan: DLL_PROCESS_ATTACH";
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        std::cout << "Renderer_Vulkan: DLL_PROCESS_DETACH";
        break;
    }
    case DLL_THREAD_ATTACH:
    {
        std::cout << "Renderer_Vulkan: DLL_THREAD_ATTACH";
        break;
    }
    case DLL_THREAD_DETACH:
    {
        std::cout << "Renderer_Vulkan: DLL_THREAD_DETACH";
        break;
    }
    }
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

vulkanBackend::vulkanBackend()
{
    instance = createInstance();
}
vulkanBackend::~vulkanBackend()
{
}

VkInstance vulkanBackend::createInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Fleur Engine";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 4, 335);
    appInfo.pEngineName = "Fleur Engine";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    appInfo.apiVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    VkExtensionProperties* props = new VkExtensionProperties[extensionCount];
    std::cout << "\nVulkan available extensions:\n";
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, props);
    for (size_t i = 0; i < extensionCount; i++)
    {
        std::cout << '\t' << props[i].extensionName << " v:" << props[i].specVersion << '\n';
    }
    delete[] props;

    uint32_t fleurExtensionCount = 0;
    const char** fleurExtensions = nullptr;

    // fleurExtensions = fleurGetRequiredInstanceExtensions(&fleurExtensionCount);

    createInfo.enabledExtensionCount = fleurExtensionCount;
    createInfo.ppEnabledExtensionNames = fleurExtensions;

    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        assert(false);
    }
    return instance;
}

void vulkanBackend::Draw(DrawInfo info)
{
}
