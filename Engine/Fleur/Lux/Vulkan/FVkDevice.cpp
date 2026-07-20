#include "FVkDevice.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cassert>
#include <iostream>
#include <set>
#include <stdexcept>

#include "VkHelper.h"

FVkDevice::FVkDevice(VkPhysicalDevice physicalDevice, FleurQueueFamilies graphicsQueueFamily)
    : m_PhysicalDevice(physicalDevice)
    , m_QueueFamilies(graphicsQueueFamily)
    , m_ForceAlpha(true)
{
}

FVkDevice::~FVkDevice()
{
    vkDestroyDevice(m_Device, nullptr);
}

VkDevice FVkDevice::CreateLogicalDevice(std::vector<const char*>& deivceExtensions)
{
    std::array<float, 2> queuePriority{1.0f, 1.0f};
    uint32_t queueCount = 2;
    std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfo{};
    if (m_QueueFamilies.m_GraphicsFamily.m_Idx != m_QueueFamilies.m_PresentFamily.m_Idx)
    {
        // GraphicsQueue
        VkDeviceQueueCreateInfo graphicsFamilyQueueCreateInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                              .queueFamilyIndex = (uint32_t)m_QueueFamilies.m_GraphicsFamily.m_Idx,
                                                              .queueCount = 1,
                                                              .pQueuePriorities = queuePriority.data()};

        deviceQueueCreateInfo.push_back(graphicsFamilyQueueCreateInfo);

        // PresentQueue
        VkDeviceQueueCreateInfo presentFamilyQueueCreateInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                             .queueFamilyIndex = (uint32_t)m_QueueFamilies.m_PresentFamily.m_Idx,
                                                             .queueCount = 1,
                                                             .pQueuePriorities = queuePriority.data()};

        deviceQueueCreateInfo.push_back(presentFamilyQueueCreateInfo);
    }
    else
    {  // GraphicsQueue &&  // PresentQueue
        VkDeviceQueueCreateInfo familyQueueCreateInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                      .queueFamilyIndex = (uint32_t)m_QueueFamilies.m_GraphicsFamily.m_Idx,
                                                      .queueCount = 2,
                                                      .pQueuePriorities = queuePriority.data()};
        deviceQueueCreateInfo.push_back(familyQueueCreateInfo);
    }


    VkPhysicalDeviceFeatures deviceFeatures{};  // Empty for now

    VkPhysicalDeviceDescriptorIndexingFeatures indexing{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
                                                        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
                                                        .descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE,
                                                        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
                                                        .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
                                                        .descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
                                                        .descriptorBindingPartiallyBound = VK_TRUE,
                                                        .descriptorBindingVariableDescriptorCount = VK_TRUE,
                                                        .runtimeDescriptorArray = VK_TRUE};

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR, .pNext = &indexing, .dynamicRendering = VK_TRUE};

    VkDeviceCreateInfo deviceCreateInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                        .pNext = &dynamicRenderingFeature,
                                        .queueCreateInfoCount = (uint32_t)deviceQueueCreateInfo.size(),
                                        .pQueueCreateInfos = deviceQueueCreateInfo.data(),
                                        .enabledExtensionCount = (uint32_t)deivceExtensions.size(),
                                        .ppEnabledExtensionNames = deivceExtensions.data(),
                                        .pEnabledFeatures = &deviceFeatures};

    VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device));
    vkGetDeviceQueue(m_Device, m_QueueFamilies.m_GraphicsFamily.m_Idx, 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, m_QueueFamilies.m_PresentFamily.m_Idx, 1, &m_PresentQueue);

    QuerySupportedVkFormats();

    return m_Device;
}

std::unique_ptr<FVkDevice> FVkDevice::CreateSuitableDevice(VkInstance instance, SDeviceInfo& deviceInfo)
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
            return std::make_unique<FVkDevice>(physicalDevices[i], FindGraphicsQueueFamily(physicalDevices[i], deviceInfo.surface));
        }
    }

    throw std::runtime_error("No suitable Vulkan device found");
}

bool FVkDevice::IsDeviceSuitable(VkPhysicalDevice physicalDevice, SDeviceInfo& deviceInfo)
{
    FleurQueueFamilies families{};

    bool isDeviceExtensionsSupported = false;
    bool isSwapchainDetailsSupported = false;

    if (deviceInfo.presentationSupport)
        families = FindGraphicsQueueFamily(physicalDevice, deviceInfo.surface);

    bool res = false;
    if (deviceInfo.presentationSupport)
        res = families.m_GraphicsFamily.IsValid() && families.m_PresentFamily.IsValid();
    else
        res = families.m_GraphicsFamily.IsValid();

    if (!res)
        return false;

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    if (!CheckDeviceExtensionSupport(physicalDevice, deviceInfo.requiredDeviceExtensions))
        return false;

    return true;
}

FleurQueueFamilies FVkDevice::FindGraphicsQueueFamily(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    FleurQueueFamilies families{};
    VkQueueFlagBits flagToFind = VK_QUEUE_GRAPHICS_BIT;

    uint32_t queueFamilyCount = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    QueueFamiliesProperties familiesProperties{};
    familiesProperties.properties.resize(queueFamilyCount);
    familiesProperties.presentSupport.resize(queueFamilyCount);

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, familiesProperties.properties.data());
    getPresentSupport(physicalDevice, surface, familiesProperties.presentSupport);
    familiesProperties.Print();

    bool graphicsFamilyFound = false;
    for (size_t i = 0; i < queueFamilyCount; i++)
    {
        if (familiesProperties.properties[i].queueCount > 0 && familiesProperties.properties[i].queueFlags & flagToFind)
        {
            if (surface)
            {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
                if (presentSupport)
                {
                    families.m_GraphicsFamily.Set(i, familiesProperties.properties[i].queueCount, familiesProperties.properties[i].queueFlags);
                    families.m_PresentFamily.Set(i, familiesProperties.properties[i].queueCount, familiesProperties.properties[i].queueFlags);
                    break;
                }
            }
            if (!graphicsFamilyFound)
            {
                graphicsFamilyFound = true;
                families.m_GraphicsFamily.Set(i, familiesProperties.properties[i].queueCount, familiesProperties.properties[i].queueFlags);
            }
        }
    }

    if (surface && !families.m_PresentFamily.IsValid())
    {
        for (size_t i = 0; i < queueFamilyCount; i++)
        {
            if (familiesProperties.properties[i].queueCount > 0)
            {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
                if (presentSupport)
                {
                    families.m_PresentFamily.Set(i, familiesProperties.properties[i].queueCount, familiesProperties.properties[i].queueFlags);
                    break;
                }
            }
        }
    }
    return families;
}

std::string FVkDevice::QueueFlagsToString(VkQueueFlags flags)
{
    std::string result;

    auto append = [&](const char* name)
    {
        if (!result.empty())
            result += " | ";
        result += name;
    };

    if (flags & VK_QUEUE_GRAPHICS_BIT)
        append("GRAPHICS");

    if (flags & VK_QUEUE_COMPUTE_BIT)
        append("COMPUTE");

    if (flags & VK_QUEUE_TRANSFER_BIT)
        append("TRANSFER");

    if (flags & VK_QUEUE_SPARSE_BINDING_BIT)
        append("SPARSE_BINDING");

    if (flags & VK_QUEUE_PROTECTED_BIT)
        append("PROTECTED");

    if (flags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)
        append("VIDEO_DECODE");

    if (flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)
        append("VIDEO_ENCODE");

    if (flags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)
        append("OPTICAL_FLOW");

    if (result.empty())
        return "NONE";

    return result;
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

void FVkDevice::QuerySupportedVkFormats()
{
    // ---------- 4 channels ----------
    std::vector<VkFormat> fourChannelsFormats{VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM};
    for (size_t i = 0; i < fourChannelsFormats.size(); i++)
    {
        if (CheckVkFormatSupport(fourChannelsFormats[i]))
        {
            m_SupportedFormatsMap.emplace(4, fourChannelsFormats[i]);
            break;
        }

        if (i == fourChannelsFormats.size() - 1)
            assert(false);
    }


    // ---------- 3 channels ----------
    std::vector<VkFormat> threeChannelsFormats{VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_UNORM};
    for (size_t i = 0; i < fourChannelsFormats.size(); i++)
    {
        if (CheckVkFormatSupport(threeChannelsFormats[i]))
        {
            m_SupportedFormatsMap.emplace(3, threeChannelsFormats[i]);
            break;
        }

        if (i == fourChannelsFormats.size() - 1)
            assert(false);
    }
}

bool FVkDevice::CheckVkFormatSupport(VkFormat format)
{
    VkImageFormatProperties formatProperties{};
    VkResult result = vkGetPhysicalDeviceImageFormatProperties(m_PhysicalDevice,
                                                               format,                                                            // Format
                                                               VK_IMAGE_TYPE_2D,                                                  // Type
                                                               VK_IMAGE_TILING_OPTIMAL,                                           // Tiling
                                                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,  // Usage
                                                               0,                                                                 // Flags
                                                               &formatProperties);

    return result == VK_SUCCESS;
}

void FVkDevice::getPresentSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<bool>& vec)
{
    for (size_t i = 0; i < vec.size(); i++)
    {
        VkBool32 presentSupport;
        std::string str{};
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        vec[i] = (bool)presentSupport;
    }
}

void FVkDevice::QueueFamiliesProperties::Print() const
{
    assert(properties.size() > 0);
    assert(properties.size() == presentSupport.size());

    for (size_t i = 0; i < properties.size(); i++)
    {
        std::string str = presentSupport[i] == true ? "PRESENT | " : "";

        std::cout << "Available QueueFamily: " << str.c_str() << QueueFlagsToString(properties[i].queueFlags).c_str() << ", count: " << properties[i].queueCount
                  << std::endl;
    }
}

void FleurQueueFamilies::QueueFamily::Set(uint32_t familyIdx, uint32_t count, VkQueueFlags flags)
{
    m_Idx = familyIdx;
    m_Count = count;
    m_Flags = flags;
}

bool FleurQueueFamilies::QueueFamily::IsValid()
{
    return (m_Idx != -1 && m_Count != -1);
}
