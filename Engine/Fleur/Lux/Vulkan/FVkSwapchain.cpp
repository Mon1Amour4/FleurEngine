#include "FVkSwapchain.h"

#include <algorithm>
#include <array>
#include <cassert>

#include "VkHelper.h"

FVkSwapchain::FVkSwapchain()
    : m_Device(nullptr)
    , m_PhysicalDevice(nullptr)
    , m_Swapchain(nullptr)
    , m_SwapchainImageFormat(VK_FORMAT_MAX_ENUM)
    , m_SwapchainExtent({0, 0})
    , m_SwapchainImageCount(0)
{
}
FVkSwapchain::~FVkSwapchain()
{
    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
}

void FVkSwapchain::ReleaseSwapchainImageViews()
{
    for (size_t i = 0; i < m_SwapchainImageCount; i++)
    {
        vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
    }
    m_SwapchainImageViews.clear();
    m_SwapchainCreated = false;
}

void FVkSwapchain::OnWindowResized(Fleur::SRect& rect)
{
    m_SwapchainCreated = false;
    m_SwapchainExtent.width = rect.width;
    m_SwapchainExtent.height = rect.height;
}

void FVkSwapchain::CreateSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, Fleur::SRect rect, uint32_t graphicsQueueFamily)
{
    m_Device = device;
    m_PhysicalDevice = physicalDevice;

    QuerySwapChainSupport(surface);

    uint32_t imageCount = m_SurfaceCapabilities.minImageCount + 1;
    imageCount = std::clamp(imageCount, m_SurfaceCapabilities.minImageCount, m_SurfaceCapabilities.maxImageCount);
    m_SwapchainImageCount = imageCount;

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(m_SurfaceFormats);
    m_SwapchainImageFormat = surfaceFormat.format;

    VkPresentModeKHR presentMode = ChooseSwapPresentMode(m_PresentModes);
    m_SwapchainExtent = ChooseSwapExtent(m_SurfaceCapabilities, rect);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_SwapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {graphicsQueueFamily};

    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
    createInfo.preTransform = m_SurfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain));

    uint32_t realSwapchainImagesCount = 0;
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &realSwapchainImagesCount, nullptr);

    assert(realSwapchainImagesCount == imageCount);

    m_SwapchainImages.resize(realSwapchainImagesCount);
    m_SwapchainImageViews.resize(realSwapchainImagesCount);
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &realSwapchainImagesCount, m_SwapchainImages.data());

    for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
    {
        m_SwapchainImageViews[i] = CreateSwapchainImageView(m_SwapchainImages[i], m_SwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    m_SwapchainCreated = true;
}


void FVkSwapchain::Recreate(VkSurfaceKHR surface, uint32_t graphicsQueueFamilyIdx, VkImageView multisampler, VkImageView depth)
{
    // 1. Release ImageViews
    ReleaseSwapchainImageViews();

    // 2. Release Swapchain
    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

    // 3. Create swapchain
    CreateSwapchain(m_Device, m_PhysicalDevice, surface, {0, 0, m_SwapchainExtent.width, m_SwapchainExtent.height}, graphicsQueueFamilyIdx);
}

bool FVkSwapchain::SwapchainPresentationSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

    return (presentModeCount > 0 && formatCount > 0);
}

void FVkSwapchain::QuerySwapChainSupport(VkSurfaceKHR surface)
{
    // Basic surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, surface, &m_SurfaceCapabilities);

    // Supported surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        m_SurfaceFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, surface, &formatCount, m_SurfaceFormats.data());
    }

    // Supported present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        m_PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, surface, &presentModeCount, m_PresentModes.data());
    }
}
VkSurfaceFormatKHR FVkSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}
VkPresentModeKHR FVkSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == /*VK_PRESENT_MODE_MAILBOX_KHR*/ VK_PRESENT_MODE_FIFO_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D FVkSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Fleur::SRect rect)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = {rect.width, rect.height};
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

VkImageView FVkSwapchain::CreateSwapchainImageView(VkImage image, VkFormat format, VkImageAspectFlagBits aspect)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    VK_CHECK(vkCreateImageView(m_Device, &viewInfo, nullptr, &imageView));

    return imageView;
}
