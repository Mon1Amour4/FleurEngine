#pragma once

#include <vulkan/vulkan.h>

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include "FVkBuffer.h"
#include "FVkCommand.h"

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


    // ---------- pImpl ----------
    VkDevice CreateLogicalDevice(std::vector<const char*>& deivceExtensions);

    [[nodiscard]] static FVkDevice* CreateSuitableDevice(VkInstance instance, SDeviceInfo& deviceInfo);

    inline VkDevice GetLogicalDevice() const
    {
        return m_Device;
    }
    inline VkPhysicalDevice GetPhysicalDevice() const
    {
        return m_PhysicalDevice;
    }
    inline uint32_t GetGraphicsQueueFamilyIndex() const
    {
        return m_GraphicsQueueFamily.familyIndex;
    }
    inline VkQueue GetGraphicsQueue() const
    {
        return m_GraphicsQueue;
    }
    inline VkQueue GetPresentQueue() const
    {
        return m_PresentQueue;
    }

private:
    static bool IsDeviceSuitable(VkPhysicalDevice physicalDevice, SDeviceInfo& deviceInfo);
    static SQueueFamily FindGraphicsQueueFamily(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    static bool CheckDeviceExtensionSupport(VkPhysicalDevice m_LogicalDevice, std::vector<const char*>& requiredDeviceExtensions);
    void QuerySupportedVkFormats();
    bool CheckVkFormatSupport();

    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;
    bool m_SwapchainSupport;
    SQueueFamily m_GraphicsQueueFamily;

    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;

    std::list<FVkBuffer> m_StagingBuffers;

    std::unordered_map<uint32_t, VkFormat> m_SupportedFormatsMap;
};
