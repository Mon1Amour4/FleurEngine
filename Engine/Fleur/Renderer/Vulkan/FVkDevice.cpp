#include "FVkDevice.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cassert>
#include <set>

bool SQueueFamily::IsValid()
{
    return (familyIndex != -1 && queueFamiliesCount != -1);
}


FVkDevice::FVkDevice(VkPhysicalDevice physicalDevice, SQueueFamily graphicsQueueFamily)
    : m_PhysicalDevice(physicalDevice)
    , m_GraphicsQueueFamily(graphicsQueueFamily)
    , m_FrameCommandBuffer(nullptr)
{
}

FVkDevice::~FVkDevice()
{
    delete m_FrameCommandBuffer;
    vkDestroyDevice(m_Device, nullptr);
}

VkDevice FVkDevice::CreateLogicalDevice(std::vector<const char*>& deivceExtensions)
{
    std::array<float, 2> queuePriority{1.0f, 1.0f};
    uint32_t queueCount = 2;
    VkDeviceQueueCreateInfo uniqueFamilyQueueCreateInfo{};
    uniqueFamilyQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    uniqueFamilyQueueCreateInfo.queueFamilyIndex = m_GraphicsQueueFamily.familyIndex;
    uniqueFamilyQueueCreateInfo.queueCount = 2;  // Queues count in this Family
    uniqueFamilyQueueCreateInfo.pQueuePriorities = queuePriority.data();

    VkPhysicalDeviceFeatures deviceFeatures{};  // Empty for now

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &uniqueFamilyQueueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;            // Unique QueueFamilies count
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;  // Pointer to array of Unique QueueFamilies
    deviceCreateInfo.ppEnabledExtensionNames = deivceExtensions.data();
    deviceCreateInfo.enabledExtensionCount = deivceExtensions.size();

    if (vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device) != VK_SUCCESS)
    {
        assert(true);
    }
    vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily.familyIndex, 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily.familyIndex, 1, &m_PresentQueue);

    return m_Device;
}

FVkDevice* FVkDevice::CreateSuitableDevice(VkInstance instance, SDeviceInfo& deviceInfo)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        assert(false);

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    // Sort devices so we can bring GPU\dGPU to the first place
    std::vector<VkPhysicalDevice> nonDiscreateGPU;
    std::vector<VkPhysicalDevice> discreateGPU;
    for (size_t i = 0; i < deviceCount; i++)
    {
        VkPhysicalDeviceProperties deviceProperties{};
        vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
        if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            nonDiscreateGPU.push_back(physicalDevices[i]);
        else
            discreateGPU.push_back(physicalDevices[i]);
    }
    physicalDevices.clear();
    physicalDevices.assign(discreateGPU.begin(), discreateGPU.end());
    physicalDevices.insert(physicalDevices.end(), nonDiscreateGPU.begin(), nonDiscreateGPU.end());

    for (size_t i = 0; i < deviceCount; i++)
    {
        if (IsDeviceSuitable(physicalDevices[i], deviceInfo))
        {
            return new FVkDevice(physicalDevices[i], FindGraphicsQueueFamily(physicalDevices[i], deviceInfo.surface));
        }
    }

    assert(false);
}

FVkSingleTimeCommandBuffer* FVkDevice::CreateFrameCommandBuffer(VkCommandPool commandPool)
{
    m_FrameCommandBuffer = new FVkSingleTimeCommandBuffer(m_Device, commandPool);
    return m_FrameCommandBuffer;
}

bool FVkDevice::IsDeviceSuitable(VkPhysicalDevice physicalDevice, SDeviceInfo& deviceInfo)
{
    SQueueFamily family{};

    bool isDeviceExtensionsSupported = false;
    bool isSwapchainDetailsSupported = false;

    if (deviceInfo.presentationSupport)
        family = FindGraphicsQueueFamily(physicalDevice, deviceInfo.surface);

    if (!family.IsValid())
        return false;

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    if (!CheckDeviceExtensionSupport(physicalDevice, deviceInfo.requiredDeviceExtensions))
        return false;

    return true;
}

SQueueFamily FVkDevice::FindGraphicsQueueFamily(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    SQueueFamily family{};
    family.familyQueueFlag = VK_QUEUE_GRAPHICS_BIT;

    uint32_t queueFamilyCount = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> familyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, familyProperties.data());

    for (size_t i = 0; i < queueFamilyCount; i++)
    {
        if (familyProperties[i].queueCount > 0 && familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            if (surface)
            {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
                if (presentSupport)
                {
                    family.familyIndex = i;
                    family.queueFamiliesCount = familyProperties[i].queueCount;
                    break;
                }
            }
            family.familyIndex = i;
            family.queueFamiliesCount = familyProperties[i].queueCount;
            break;
        }
    }

    return family;
}

bool FVkDevice::CheckDeviceExtensionSupport(VkPhysicalDevice m_LogicalDevice, std::vector<const char*>& requiredDeviceExtensions)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(m_LogicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableDeviceExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(m_LogicalDevice, nullptr, &extensionCount, availableDeviceExtensions.data());

    std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());
    for (auto& ext : availableDeviceExtensions)
    {
        requiredExtensions.erase(ext.extensionName);
    }

    return requiredExtensions.empty();
}
