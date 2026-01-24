#pragma once

#pragma region Includes& Definitions

#include "Renderer_Vulkan.h"
#if defined(FLEUR_PLATFORM_WIN)
#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <vector>

#include "FVkBuffer.h"
#include "FVkCapabilities.h"
#include "FVkCommand.h"

#if defined(FL_CONF_DEBUG)
#define DBG_PRINT(moduleText, text) std::cout << moduleText << text << std::endl;
#define MODULE "[Vulkan] "
#define DBG_PRINTM(text) std::cout << MODULE << text << std::endl;
#else
#define DBG_PRINT(moduleText, text)
#define MODULE
#define DBG_PRINTM(text)
#endif

#define VULKAN_VERSION VK_API_VERSION_1_4

constexpr uint32_t MAX_TEXTURES = 128;

#pragma endregion

#pragma region Structs

struct SSwapchainSupport
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

    inline bool isSwapchainSuitable()
    {
        return (!formats.empty() && !presentModes.empty());
    }
};

struct SLogicalDevice
{
    VkPhysicalDevice vkPhysicalDevice;
    SSwapchainSupport capabilities;
};

struct SUniqueFamilyQueue
{
    int familyIndex{-1};
    uint32_t availableQueueCount{0};

    bool swapchainSupport{false};

    inline bool is_valid()
    {
        return (familyIndex != -1 && availableQueueCount > 0);
    }
};

struct SGPUMaterial
{
    uint32_t albedo;
    uint32_t normal;
};
struct DrawInfo
{
    uint64_t indexCount = 0;
    uint64_t vertexCount = 0;

    uint64_t indexOffset = 0;
    uint64_t vertexOffset = 0;

    SGPUMaterial material;
};

struct SGPUTexture
{
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
};

struct SFLSwapchain
{
    VkSwapchainKHR swapchain;
    VkFormat imageFormat;
    VkExtent2D extent;

    // VkFramebuffer + VkRenderPass defines the render target
    std::vector<VkImage> images;              // Raw GPU data
    std::vector<VkImageView> imageViews;      // Describes how to interpret that Raw GPU data
    std::vector<VkFramebuffer> framebuffers;  // Relates to single RenderPass, defines which VkImageView is to be which attachment.

    uint32_t framebuffersCount;
};

#pragma endregion

//======================================================================
// Static functions
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        DBG_PRINTM("debug callback: " << pCallbackData->pMessage);
        for (size_t i = 0; i < pCallbackData->objectCount; i++)
        {
            if (!pCallbackData->pObjects[i].pObjectName)
                break;
            DBG_PRINT("", "\t [Object] " << pCallbackData->pObjects[i].pObjectName);
        }
        std::cout << "\n";
    }

    return VK_FALSE;
}

struct vulkanBackend::vulkanBackendImpl
{
    vulkanBackendImpl(bool enableValidation, Fleur::Graphics::SFLFrame& pFrame, void* pNativeHandle, Fleur::SRect& framebufferSize,
                      Fleur::Graphics::SFLImageView& fallback);
    ~vulkanBackendImpl();

    void update(Fleur::Graphics::SFLGeometryUBO* pUbo);
    void resize_event(Fleur::SRect& rect);


    // Instance
    VkInstance m_VulkanInstance;
    VkInstance createInstance();

    FVkCapabilities* m_Capabilities;

    VkDebugUtilsMessengerEXT debugMessenger;
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setupDebugMessenger();
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                          VkDebugUtilsMessengerEXT* pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

    // Physical\Logical device
    VkDevice m_LogicalDevice;
    SLogicalDevice m_PhysicalDevice;
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice& m_LogicalDevice);

    // Queue families

    SUniqueFamilyQueue m_GraphicsQueueFamily;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    SUniqueFamilyQueue findQueueFamilies(VkPhysicalDevice m_LogicalDevice);

    // Logical device
    void createLogicalDevice();

    // Surface
    VkSurfaceKHR surface;
    void createSurface(void* pNativeHandle);
    Fleur::SRect surfaceRect;

    // Swapchain
    SFLSwapchain m_Swapchain;
    void createImageViews();
    void createFramebuffers();

    void cleanupSwapChain();
    void recreateSwapChain();


    SSwapchainSupport querySwapChainSupport(VkPhysicalDevice vkPhysicalDevice);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Fleur::SRect& framebufferSize);
    void createSwapChain(Fleur::SRect& framebufferSize);

    // GeometryPipeline
    VkPipeline m_GeometryPipeline;
    VkPipelineLayout m_GeometryPipelineLayout;
    VkDescriptorSetLayout m_GeometryDSL;
    void CreateGeometryPipeline(Fleur::Graphics::SFLShaderInfo* pVertexInfo, Fleur::Graphics::SFLShaderInfo* pFragmentInfo,
                                Fleur::Graphics::EFLInputAssemblyTopology pInputAssemblyTopology);

    // Renderpass
    VkRenderPass m_GeometryRenderPass;
    void CreateGeometryRenderPass();

    // Shaders
    VkShaderModule CreateShaderModule(Fleur::Graphics::SFLShaderInfo* pShaderInfo);

    // CommandPool
    FVkCommandPool* m_GraphicsCommandPool;

    // CommandBuffer
    struct SFLCmdBuffer
    {
        std::vector<VkCommandBuffer> buffers;
        std::vector<bool> validation;
        void Invalidate();
        bool AreValid();
    };

    std::vector<FVkCommandBuffer> m_PrimaryCmdBuffers;
    std::vector<FVkCommandBuffer> m_SecondaryCmdBuffers;
    std::vector<bool> m_SecondaryCmdValidation;
    void InitGeometryPrimaryCmdBuffers();
    void UpdateGeometrySecondaryCmdBuffer(uint32_t idx);

    // Synchronization
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    bool framebufferResized = false;
    void createSyncObjects();

    std::vector<FVkBuffer> m_UniformBuffers;

    VkMemoryRequirements memRequirements;

    void createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage, Fleur::Graphics::SFLGeometryUBO* pUbo);

    void createDescriptorSetLayout();

    VkDescriptorPool descriptorPool;
    void createDescriptorPool();

    std::vector<VkDescriptorSet> descriptorSets;
    void createDescriptorSets();

    // VMA
    VmaAllocator m_Allocator;
    void initializeVma();
    void freeVma();

    FVkBuffer* m_VertexBuffer;
    FVkBuffer* m_IndexBuffer;


    std::vector<DrawInfo> m_DrawList;
    void AddToDrawList(Fleur::Graphics::SFLModelView* pModelView);

    VkVertexInputBindingDescription GetVertexDataBindingDescriptor();
    std::array<VkVertexInputAttributeDescription, 3> GetVertexDataAttributeDescriptions();

    uint32_t currentFrame = 0;

    void CreateTextureImage(Fleur::Graphics::SFLImageView& imageView, VkImage& image, VkDeviceMemory& imageMemory, VkFormat format);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkImage& image, VkDeviceMemory& imageMemory);

    void SubmitImageViews(Fleur::Graphics::SFLImageViewInfo* pInfo);

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    VkImageView createTextureImageView(VkImage& image, VkFormat format);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkSampler createTextureSampler();

    VkSampler m_ImageSampler;
    std::unordered_map<uint32_t, SGPUTexture> m_TextureMap;
    uint32_t m_FallbackTextureIdx;
    void UpdateDescriptorSets(VkDescriptorSet& set, uint32_t idx, VkImageView& imageView, VkSampler& sampler);

    uint32_t GetChannelsNumFromFormat(VkFormat);

    struct SFLDescriptorSetImage
    {
        uint32_t idx;
        VkImageView view;
    };
    std::vector<std::vector<SFLDescriptorSetImage>> m_DescriptorSetImageViews;

    SGPUTexture m_FallbackTexture;
    void CreateFallbackTexture(Fleur::Graphics::SFLImageView& pInfo);


    // Depth
    struct Depth
    {
        Depth() = default;
        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;
    };
    VkFormat FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat FindDepthFormat(VkPhysicalDevice device);
    bool HasStencilComponent(VkFormat format);
    vulkanBackend::vulkanBackendImpl::Depth CreateDepthBuffer(VkPhysicalDevice device);
    Depth m_Depth;
};
