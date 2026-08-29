#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include <cassert>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include "FVkBuffer.h"
#include "FVkCommand.h"
#include "FVkMemoryTracker.h"

struct FleurQueueFamilies
{
    FleurQueueFamilies() = default;

    struct QueueFamily
    {
        int m_Idx;
        int m_Count;
        VkQueueFlags m_Flags;

        QueueFamily()
            : m_Idx(-1)
            , m_Count(-1)
            , m_Flags(VK_QUEUE_FLAG_BITS_MAX_ENUM) {};

        void Set(uint32_t familyIdx, uint32_t count, VkQueueFlags flags);
        bool IsValid();
    };

    QueueFamily m_GraphicsFamily;
    QueueFamily m_PresentFamily;

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
    FVkDevice(VkPhysicalDevice physicalDevice, FleurQueueFamilies graphicsQueueFamily);
    ~FVkDevice();


    // ---------- pImpl ----------
    VkDevice CreateLogicalDevice(std::vector<const char*>& deivceExtensions);

    [[nodiscard]] static std::unique_ptr<FVkDevice> CreateSuitableDevice(VkInstance instance, SDeviceInfo& deviceInfo);

    inline VkDevice GetLogicalDevice() const
    {
        return m_Device;
    }
    inline VkPhysicalDevice GetPhysicalDevice() const
    {
        return m_PhysicalDevice;
    }
    inline int GetGraphicsQueueFamilyIndex() const
    {
        return m_QueueFamilies.m_GraphicsFamily.m_Idx;
    }
    inline int GetPresentQueueFamilyIndex() const
    {
        return m_QueueFamilies.m_PresentFamily.m_Idx;
    }
    inline VkQueue GetGraphicsQueue() const
    {
        return m_GraphicsQueue;
    }
    inline VkQueue GetPresentQueue() const
    {
        return m_PresentQueue;
    }
    inline VkFormat GetTextureFormat(uint32_t channels, bool srgb = true)
    {
        assert(channels <= 4);

        if (!srgb)
        {
            if (channels == 4 || channels == 3)
                return VK_FORMAT_R8G8B8A8_UNORM;
        }

        if (m_ForceAlpha)
            return m_SupportedFormatsMap[4];

        assert(m_SupportedFormatsMap.contains(channels));

        return m_SupportedFormatsMap[channels];
    }

    inline FVkMemoryTracker& GetMemoryTracker() const
    {
        return const_cast<FVkMemoryTracker&>(m_MemoryTracker);
    }

private:
    static bool IsDeviceSuitable(VkPhysicalDevice physicalDevice, SDeviceInfo& deviceInfo);
    static FleurQueueFamilies FindGraphicsQueueFamily(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    static std::string QueueFlagsToString(VkQueueFlags flags);
    static bool CheckDeviceExtensionSupport(VkPhysicalDevice m_LogicalDevice, std::vector<const char*>& requiredDeviceExtensions);
    void QuerySupportedVkFormats();
    bool CheckVkFormatSupport(VkFormat format);

    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;
    bool m_SwapchainSupport;

    FleurQueueFamilies m_QueueFamilies;

    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;

    FVkMemoryTracker m_MemoryTracker;
    std::list<FVkBuffer> m_StagingBuffers;

    std::unordered_map<uint32_t, VkFormat> m_SupportedFormatsMap;

    bool m_ForceAlpha;

    struct QueueFamiliesProperties
    {
        std::vector<bool> presentSupport;
        std::vector<VkQueueFamilyProperties> properties;

        void Print() const;
    };
    QueueFamiliesProperties m_QueueFamilySupport;
    static void getPresentSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<bool>& vec);
};
