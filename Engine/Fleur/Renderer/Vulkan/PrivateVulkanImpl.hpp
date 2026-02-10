#pragma once

#pragma region Includes& Definitions

#include "Renderer_Vulkan.h"


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
#include "FVkCubemap.h"
#include "FVkDevice.h"
#include "FVkMultisampler.h"
#include "FVkPipeline.h"
#include "FVkSwapchain.h"
#include "FVkTexture.h"
#include "VkHelper.h"

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
#define CUBEMAP_LAYERS_COUNT 6

constexpr uint32_t MAX_TEXTURES = 128;

#pragma endregion

#pragma region Structs

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

    void SetSkybox(AssetID id);

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

    Fleur::SRect m_SurfaceRect;

    FVkDevice* m_Device;
    FVkSwapchain* m_Swapchain;
    VkSurfaceKHR m_Surface;
    VkSurfaceKHR CreateSurface(VkInstance instance, void* nativeHandle);
    std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // GeometryPipeline
    VkDescriptorSetLayout m_GeometryDSL;
    FVkPipeline* m_GeometryPipeline;
    FVkPipeline* CreateGeometryPipeline(Fleur::Graphics::SFLShaderInfo* pVertexInfo, Fleur::Graphics::SFLShaderInfo* pFragmentInfo,
                                        Fleur::Graphics::EFLInputAssemblyTopology pInputAssemblyTopology, VkSampleCountFlagBits samplesCount);

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
    FVkCommandBuffer m_SkyboxCmd;

    std::vector<FVkCommandBuffer> m_SecondaryCmdBuffers;
    std::vector<bool> m_SecondaryCmdValidation;
    void InitGeometryPrimaryCmdBuffers(uint32_t idx);
    void UpdateGeometrySecondaryCmdBuffer(uint32_t idx);

    // Synchronization
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
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

    void SubmitImageViews(Fleur::Graphics::SFLImageViewInfo* pInfo);

    VkImageView createTextureImageView(VkImage& image, VkFormat format);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkSampler createTextureSampler();

    VkSampler m_ImageSampler;
    std::unordered_map<AssetID, FVkTexture> m_TextureMap;
    uint32_t m_FallbackTextureIdx;
    void UpdateDescriptorSets(VkDescriptorSet& set, uint32_t idx, VkImageView imageView, VkSampler& sampler);

    struct SFLDescriptorSetImage
    {
        uint32_t idx;
        VkImageView view;
    };
    std::vector<std::vector<SFLDescriptorSetImage>> m_DescriptorSetImageViews;

    FVkTexture* m_FallbackTexture;
    void CreateFallbackTexture(Fleur::Graphics::SFLImageView& pInfo);


    // Depth
    struct Depth
    {
        Depth() = default;
        FVkTexture* depthTexture;
    };
    void CreateDepthBuffer(vulkanBackend::vulkanBackendImpl::Depth& depthBuffer, VkPhysicalDevice device, VkSampleCountFlagBits samplesCount,
                           uint32_t mimLevels);
    Depth m_Depth;

    SFLVertexInput* m_GeometryVertexInput;
    FVkMultisampler* m_Multisampler;

    void CreateTexture(FVkTexture& texture, Fleur::Graphics::SFLImageView& view, VkFormat format, VkImageAspectFlags aspect, uint32_t mipLevels);

    void CreateDepthTexture(FVkTexture& texture, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits samplesCount, uint32_t mimLevels);

    void StartResize();
    void EndResize(Fleur::SRect& rect);

    bool m_WindowResizeIsInProgress;

    FVkSkybox* m_Skybox;
    void CreateSkybox(AssetID id, SFLShaderInfo* pVertexShaderInfo, SFLShaderInfo* pFragmentShaderInfo);
};
