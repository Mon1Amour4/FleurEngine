#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

struct SQueueFamily
{
    SQueueFamily()
        : familyIndex(-1)
        , queueFamiliesCount(-1)
        , familyQueueFlag(VK_QUEUE_FLAG_BITS_MAX_ENUM) {};

    int familyIndex{};
    int queueFamiliesCount{};
    VkQueueFlagBits familyQueueFlag{};

    bool IsValid();
};

struct SDeviceInfo
{
    VkQueueFlagBits neededQueueFamilyFlags;

    bool presentationSupport;
    VkSurfaceKHR surface;
    std::vector<const char*> requiredDeviceExtensions;

};

class FVkDevice
{
public:
    FVkDevice(VkPhysicalDevice physicalDevice, SQueueFamily graphicsQueueFamily);
    ~FVkDevice();

    VkDevice CreateLogicalDevice(std::vector<const char*>& deivceExtensions);

    [[nodiscard]] static FVkDevice* CreateSuitableDevice(VkInstance instance, SDeviceInfo& deviceInfo);

    inline VkDevice GetLogicalDevice()
    {
        return m_Device;
    }
    inline VkPhysicalDevice GetPhysicalDevice()
    {
        return m_PhysicalDevice;
    }
    inline uint32_t GetGraphicsQueueFamilyIndex()
    {
        return m_GraphicsQueueFamily.familyIndex;
    }
    inline VkQueue GetGraphicsQueue()
    {
        return m_GraphicsQueue;
    }
    inline VkQueue GetPresentQueue()
    {
        return m_PresentQueue;
    }

private:
    static bool IsDeviceSuitable(VkPhysicalDevice physicalDevice, SDeviceInfo& deviceInfo);
    static SQueueFamily FindGraphicsQueueFamily(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    static bool CheckDeviceExtensionSupport(VkPhysicalDevice m_LogicalDevice, std::vector<const char*>& requiredDeviceExtensions);

    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;
    bool m_SwapchainSupport;
    SQueueFamily m_GraphicsQueueFamily;

    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;
};