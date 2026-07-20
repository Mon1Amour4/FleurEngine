#pragma once

#include <Vulkan/vulkan.h>

#include <cassert>
#include <vector>

#include "../WindowPrimitives.hpp"

class FVkSwapchain
{
public:
    FVkSwapchain();
    ~FVkSwapchain();

    void CreateSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, Fleur::SRect rect, uint32_t graphicsQueueFamily);
    void Recreate(VkSurfaceKHR surface, uint32_t graphicsQueueFamilyIdx, VkImageView multisampler, VkImageView depth);

    bool SwapchainPresentationSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    inline VkSwapchainKHR GetSwapchain() const
    {
        return m_Swapchain;
    }
    inline VkFormat GetImageFormat() const
    {
        return m_SwapchainImageFormat;
    }
    inline VkExtent2D GetSwapchainExtent() const
    {
        return m_SwapchainExtent;
    }
    inline uint32_t GetSwapchainImageCount() const
    {
        return m_SwapchainImageCount;
    }
    inline bool ReadyToPresent()
    {
        return (m_SwapchainCreated);
    }
    inline VkImage GetSwapchainImage(uint32_t idx) const
    {
        assert(idx <= m_SwapchainImages.size() - 1);

        return m_SwapchainImages[idx];
    }
    inline VkImageView GetSwapchainImageView(uint32_t idx) const
    {
        assert(idx <= m_SwapchainImages.size() - 1);

        return m_SwapchainImageViews[idx];
    }

    void ReleaseSwapchainImageViews();

    void OnWindowResized(Fleur::SRect& rect);

private:
    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;
    VkSwapchainKHR m_Swapchain;
    VkFormat m_SwapchainImageFormat;

    VkExtent2D m_SwapchainExtent;

    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;

    VkSurfaceCapabilitiesKHR m_SurfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> m_SurfaceFormats;
    std::vector<VkPresentModeKHR> m_PresentModes;

    bool m_SwapchainCreated;

    uint32_t m_SwapchainImageCount;

    inline bool isSwapchainSuitable()
    {
        return (!m_SurfaceFormats.empty() && !m_PresentModes.empty());
    }


    void QuerySwapChainSupport(VkSurfaceKHR surface);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Fleur::SRect rect);

    VkImageView CreateSwapchainImageView(VkImage, VkFormat, VkImageAspectFlagBits);
};
