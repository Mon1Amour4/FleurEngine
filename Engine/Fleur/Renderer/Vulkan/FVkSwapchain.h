#pragma once 

#include <vector>

#if defined(FLEUR_PLATFORM_WIN)
#define NOMINMAX
//#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <Vulkan/vulkan.h>

#include "../WindowPrimitives.hpp"

class FVkSwapchain
{
public:
    FVkSwapchain();
    ~FVkSwapchain();

    void CreateSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, Fleur::SRect rect, uint32_t graphicsQueueFamily);
    VkSurfaceKHR CreateSurface(VkInstance instance, void* pNativeHandle);
    void CreateFrameBuffers(VkRenderPass renderPass, VkImageView multisampler, VkImageView depth);
    void Recreate(Fleur::SRect rect);

    bool SwapchainPresentationSupport(VkPhysicalDevice physicalDevice);

    inline VkFramebuffer GetFramebuffer(uint32_t idx)
    {
        assert(idx <= m_FramebuffersCount - 1);
        return m_Framebuffers[idx];
    }
    inline VkSwapchainKHR GetSwapchain()
    {
        return m_Swapchain;
    }
    inline VkSurfaceKHR GetSurface()
    {
        return m_Surface;
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

private:
    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;
    VkSwapchainKHR m_Swapchain;
    VkFormat m_SwapchainImageFormat;
    VkSurfaceKHR m_Surface;
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


    void QuerySwapChainSupport();
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Fleur::SRect rect);

    VkImageView CreateSwapchainImageView(VkImage, VkFormat, VkImageAspectFlagBits);
};