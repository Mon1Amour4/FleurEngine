#include <cassert>
#include <algorithm>
#include <array>

#include "FVkSwapchain.h"

FVkSwapchain::FVkSwapchain()
    : m_Device(nullptr)
    , m_PhysicalDevice(nullptr)
    , m_Swapchain(nullptr)
    , m_SwapchainImageFormat(VK_FORMAT_MAX_ENUM)
    , m_Surface(nullptr)
    , m_SwapchainExtent({0, 0})
    , m_FramebuffersCount(0)
{
}
FVkSwapchain::~FVkSwapchain()
{
    for (size_t i = 0; i < m_FramebuffersCount; i++)
    {
        vkDestroyFramebuffer(m_Device, m_Framebuffers[i], nullptr);
        vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
    }
    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
}

void FVkSwapchain::CreateSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, Fleur::SRect rect, uint32_t graphicsQueueFamily)
{
    m_Device = device;
    m_PhysicalDevice = physicalDevice;

    QuerySwapChainSupport();

    uint32_t imageCount = m_SurfaceCapabilities.minImageCount + 1;
    imageCount = std::clamp(imageCount, m_SurfaceCapabilities.minImageCount, m_SurfaceCapabilities.maxImageCount);
    m_FramebuffersCount = imageCount;

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(m_SurfaceFormats);
    m_SwapchainImageFormat = surfaceFormat.format;

    VkPresentModeKHR presentMode = ChooseSwapPresentMode(m_PresentModes);
    m_SwapchainExtent = ChooseSwapExtent(m_SurfaceCapabilities, rect);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Surface;
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

    if (vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS)
        assert(false);

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
}

VkSurfaceKHR FVkSwapchain::CreateSurface(VkInstance instance, void* pNativeHandle)
{
#if defined(FLEUR_PLATFORM_WIN) 
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = reinterpret_cast<HWND>(pNativeHandle);
    createInfo.hinstance = GetModuleHandle(nullptr);

    if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &m_Surface) != VK_SUCCESS)
    {
        assert(false);
    }
#endif

    return m_Surface;
}

void FVkSwapchain::CreateFrameBuffers(VkRenderPass renderPass, VkImageView multisampler, VkImageView depth)
{
    m_Framebuffers.resize(m_FramebuffersCount);

    for (size_t i = 0; i < m_Framebuffers.size(); i++)
    {
        std::array<VkImageView, 3> attachments = {multisampler, depth, m_SwapchainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_SwapchainExtent.width;
        framebufferInfo.height = m_SwapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS)
            assert(false);
    }
}

void FVkSwapchain::Recreate(Fleur::SRect rect)
{
   /* vkDeviceWaitIdle(m_LogicalDevice);

    cleanupSwapChain();

    createSwapChain(surfaceRect);
    createImageViews();
    createFramebuffers();*/
}

bool FVkSwapchain::SwapchainPresentationSupport(VkPhysicalDevice physicalDevice)
{
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, nullptr);

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, nullptr);

    return (presentModeCount > 0 && formatCount > 0);
}

void FVkSwapchain::QuerySwapChainSupport()
{
    // Basic surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &m_SurfaceCapabilities);

    // Supported surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        m_SurfaceFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, m_SurfaceFormats.data());
    }

    // Supported present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        m_PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, m_PresentModes.data());
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
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
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
    if (vkCreateImageView(m_Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        assert(true);

    return imageView;
}
