#include "VkCapabilities.h"
#include <string>
#include <set>

VkCapabilities::VkCapabilities(bool enableValidation)
    : enableValidationLayers(enableValidation)
{
#if defined(FLEUR_PLATFORM_WIN) 
    instanceExtensions.emplace_back("VK_KHR_win32_surface");
#endif
}

void VkCapabilities::EnableValidationLayersSupport(VkInstanceCreateInfo& createinfo)
{
    uint32_t availableLayerCount;
    vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(availableLayerCount);
    vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());
    /*DBG_PRINTM("Vulkan available validation layers:");
    for (size_t i = 0; i < availableLayerCount; i++)
    {
        DBG_PRINT("", '\t' << availableLayers[i].layerName << "  spec_v: " << availableLayers[i].specVersion
                           << "impl_v: " << availableLayers[i].implementationVersion << ' ' << availableLayers[i].description);
    }*/

    createinfo.enabledLayerCount = validationLayers.size();
    createinfo.ppEnabledLayerNames = validationLayers.data();
}

void VkCapabilities::EnableExtensions(VkInstanceCreateInfo& createinfo)
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> props(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, props.data());

   /* DBG_PRINTM("Vulkan available extensions:");
    for (size_t i = 0; i < extensionCount; i++)
    {
        DBG_PRINT("", '\t' << props[i].extensionName << " v:" << props[i].specVersion);
    }*/

    createinfo.enabledExtensionCount = instanceExtensions.size();
    createinfo.ppEnabledExtensionNames = instanceExtensions.data();
}

bool VkCapabilities::CheckDeviceExtensionSupport(VkPhysicalDevice m_LogicalDevice)
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
