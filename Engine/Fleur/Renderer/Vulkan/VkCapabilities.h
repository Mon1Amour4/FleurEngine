#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class VkCapabilities
{
public:
    VkCapabilities(bool enableValidation);
    void EnableValidationLayersSupport(VkInstanceCreateInfo& createinfo);
    void EnableExtensions(VkInstanceCreateInfo& createinfo);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice m_LogicalDevice);
    inline bool ValidationEnabled() const
    {
        return enableValidationLayers;
    }

    inline const char* const* DeviceExtensionsData() const
    {
        return deviceExtensions.data();
    }
    inline uint32_t DeviceExtensionsCount() const
    {
        return deviceExtensions.size();
    }

    private:
    bool enableValidationLayers;

    std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    std::vector<const char*> instanceExtensions = {"VK_EXT_debug_utils", "VK_KHR_surface"};
    std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};
};