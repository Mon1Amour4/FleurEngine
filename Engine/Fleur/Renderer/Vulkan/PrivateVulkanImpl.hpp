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

    struct SFLBuffer
    {
        uint64_t sizeBytes = 0;
        uint64_t currentSizeBytes = 0;
        uint32_t strideSizeBytes = 0;
        VkBuffer buffer;
        VmaAllocation allocation;
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
        // VkTexture* albedo;
        // VkTexture* normal;
    };
    struct DrawInfo
    {
        uint64_t indexCount = 0;
        uint64_t vertexCount = 0;

        uint64_t indexOffset = 0;
        uint64_t vertexOffset = 0;

        SGPUMaterial material;
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

    // CPU ID to corresponding Vulkan Texture
    // std::unordered_map<uint32_t, VkTexture> m_TextureMap;
#pragma endregion

    vulkanBackendImpl(Fleur::Graphics::SFLFrame* pFrame, void* pNativeHandle, Fleur::SRect& framebufferSize);
    ~vulkanBackendImpl();

    void update(Fleur::Graphics::SFLGeometryUBO* pUbo);
    void resize_event(Fleur::SRect& rect);

    // Extensions
    std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    std::vector<const char*> instanceExtensions = {"VK_EXT_debug_utils", "VK_KHR_surface"};

    // Instance
    VkInstance m_VulkanInstance;
    VkInstance createInstance();

    // Validation Layers
    void enableValidationLayersSupport(VkInstanceCreateInfo& createinfo);

    // Extensions
    void enableExtensions(VkInstanceCreateInfo& createinfo);
    bool checkDeviceExtensionSupport(VkPhysicalDevice m_LogicalDevice);

    // Debug messages
    bool enableValidationLayers;
    std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};

    VkDebugUtilsMessengerEXT debugMessenger;
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setupDebugMessenger();
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                          VkDebugUtilsMessengerEXT* pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

    // Physical\Logical device
    VkDevice m_LogicalDevice;
    SLogicalDevice m_SPhysicalDevice;
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
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);


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
    VkCommandPool commandPool;
    void createCommandPool();

    // CommandBuffer
    struct SFLCmdBuffer
    {
        std::vector<VkCommandBuffer> buffers;
        std::vector<bool> validation;
        void Invalidate();
        bool AreValid();
    };
    SFLCmdBuffer m_PrimaryCmdBuffers;
    VkCommandBuffer m_GeometrySecondaryCmdBuffer;
    void createCommandBuffers();
    VkCommandBuffer CreateCmdBuffer(VkCommandBufferLevel level);
    void InitGeometryPrimaryCmdBuffers();
    void UpdateGeometryPrimaryBuffer(uint32_t bufferIdx);
    void UpdateGeometrySecondaryCmdBuffer();

    // Synchronization
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    bool framebufferResized = false;
    void createSyncObjects();

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkMemoryRequirements memRequirements;

    void CreateBuffer(VkBufferUsageFlags usage, SFLBuffer* pBuffer, VkDeviceSize sizeBytes, VkDeviceSize strideSize);
    void createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage, Fleur::Graphics::SFLGeometryUBO* pUbo);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createDescriptorSetLayout();

    VkDescriptorPool descriptorPool;
    void createDescriptorPool();

    std::vector<VkDescriptorSet> descriptorSets;
    void createDescriptorSets();

    // VMA
    VmaAllocator m_Allocator;
    void initializeVma();
    void freeVma();

    SFLBuffer m_VertexBuffer;
    SFLBuffer m_IndexBuffer;

    void UploadDataToBuffer(SFLBuffer* pBuffer, const void* pData, uint64_t count);


    std::vector<DrawInfo> m_DrawList;
    void AddToDrawList(Fleur::Graphics::SFLModelView* pModelView);
    bool needToUpdateSecondaryCmdBuffer = false;

    VkVertexInputBindingDescription GetVertexDataBindingDescriptor();
    std::array<VkVertexInputAttributeDescription, 3> GetVertexDataAttributeDescriptions();

    uint32_t currentFrame = 0;

    void CreateTextureImage(Fleur::Graphics::SFLImageView& imageView);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkImage& image, VkDeviceMemory& imageMemory);
    struct Texture
    {
        const char* pData = nullptr;
        uint32_t w = 0;
        uint32_t h = 0;
        uint32_t layerCount = 0;
    };
    std::unordered_map<uint32_t, Texture> m_TextureMap;
    void SubmitImageViews(Fleur::Graphics::SFLImageViewInfo* pInfo);
};
