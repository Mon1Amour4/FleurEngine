#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class FVkCapabilities
{
public:
    FVkCapabilities(bool enableValidation);
    void EnableValidationLayersSupport(VkInstanceCreateInfo& createinfo);
    void EnableExtensions(VkInstanceCreateInfo& createinfo);
    inline bool ValidationEnabled() const
    {
        return enableValidationLayers;
    }

    private:
    bool enableValidationLayers;

    std::vector<const char*> instanceExtensions = {"VK_EXT_debug_utils", "VK_KHR_surface"};
    std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};
};