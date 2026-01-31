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
    void CreateFrameBuffers(VkRenderPass renderPass, VkImageView multisampler, VkImageView depth);
    void Recreate(Fleur::SRect rect);

    bool SwapchainPresentationSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    inline VkFramebuffer GetFramebuffer(uint32_t idx)
    {
        assert(idx <= m_FramebuffersCount - 1);
        return m_Framebuffers[idx];
    }
    inline VkSwapchainKHR GetSwapchain()
    {
        return m_Swapchain;
    }
    inline VkFormat GetImageFormat()
    {
        return m_SwapchainImageFormat;
    }
    inline VkExtent2D GetSwapchainExtent()
    {
        return m_SwapchainExtent;
    }
    inline uint32_t GetSwapchainFramebuffersCount()
    {
        return m_FramebuffersCount;
    }

    void ReleaseFramebuffers();
    void ReleaseSwapchainImageViews();

private:
    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;
    VkSwapchainKHR m_Swapchain;
    VkFormat m_SwapchainImageFormat;

    VkExtent2D m_SwapchainExtent;

    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;
    std::vector<VkFramebuffer> m_Framebuffers;

    VkSurfaceCapabilitiesKHR m_SurfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> m_SurfaceFormats;
    std::vector<VkPresentModeKHR> m_PresentModes;

    uint32_t m_FramebuffersCount;

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