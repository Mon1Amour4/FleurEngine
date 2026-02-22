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
#include "FVkCommand.h"
#include "FVkDevice.h"
#include "FVkMultisampler.h"
#include "FVkPipeline.h"
#include "FVkShader.h"
#include "FVkSkybox.h"
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

constexpr uint32_t MAX_TEXTURES = 4096;

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

// ---------- static function ----------
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

namespace vk
{

struct backend::impl
{
    impl(bool enableValidation, Fleur::Graphics::SFLFrame& pFrame, void* pNativeHandle, Fleur::SRect& framebufferSize, Fleur::Graphics::SFLImageView& fallback);
    ~impl();

    void update(Fleur::Graphics::SFLCameraData cameraData);

    // Instance
    struct
    {
        int Major;
        int Minor;
        int Patch;
    } m_InstanceVersion;
    VkInstance m_VulkanInstance;
    bool m_ValidationsEnabled{false};
    VkInstance createInstance(bool enableValidation, const std::vector<const char*>& instanceExtensions, const std::vector<const char*>& validationLayers);

    VkDebugUtilsMessengerEXT debugMessenger;
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setupDebugMessenger();
    VkResult createDebugUtilsMessenger_EXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                           VkDebugUtilsMessengerEXT* pDebugMessenger);
    void destroyDebugUtilsMessenger_EXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

    Fleur::SRect m_SurfaceRect;

    FVkDevice* m_Device;
    FVkSwapchain* m_Swapchain;
    VkSurfaceKHR m_Surface;
    VkSurfaceKHR createSurface(VkInstance instance, void* nativeHandle);
    std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME};

    // ---------- geometryPipeline ----------

    FVkPipeline* m_GeometryPipeline;
    FVkPipeline* createGeometryPipeline(Fleur::Graphics::SFLShaderInfo* pVertexInfo, Fleur::Graphics::SFLShaderInfo* pFragmentInfo,
                                        Fleur::Graphics::EFLInputAssemblyTopology pInputAssemblyTopology, VkSampleCountFlagBits samplesCount);


    // ---------- shaders ----------
    VkShaderModule createShaderModule(Fleur::Graphics::SFLShaderInfo* pShaderInfo);


    // ---------- commandBuffer ----------
    struct SFLCmdBuffer
    {
        std::vector<VkCommandBuffer> buffers;
        std::vector<bool> validation;
        void invalidate();
        bool areValid();
    };

    FVkCommandBuffer m_SkyboxCmd;

    struct FrameContext
    {
        FrameContext(uint32_t framesInFlight)
            : m_FramesInFlight(framesInFlight)
            , m_CommandPools(m_FramesInFlight)
            , m_CommandBuffers(m_FramesInFlight)
            , m_InFlightFences(m_FramesInFlight)
            , m_ImagesAvailable(m_FramesInFlight)
            , m_RenderFinished(m_FramesInFlight)
            , m_FrameCommandPool(nullptr) {};

        uint32_t m_FramesInFlight;
        std::vector<FVkCommandPool> m_CommandPools;
        std::vector<FVkCommandBuffer> m_CommandBuffers;
        std::vector<VkFence> m_InFlightFences;
        std::vector<VkSemaphore> m_ImagesAvailable;
        std::vector<VkSemaphore> m_RenderFinished;

        FVkCommandPool* m_FrameCommandPool;
    };
    FrameContext* m_FrameContext;


    VkMemoryRequirements memRequirements;


    // ---------- vma ----------
    VmaAllocator m_Allocator;
    void initializeVma();
    void freeVma();

    FVkBuffer* m_VertexBuffer;
    FVkBuffer* m_IndexBuffer;

    std::vector<DrawInfo> m_DrawList;
    void addToDrawList(Fleur::Graphics::SFLModelView* pModelView);

    uint32_t currentFrame = 0;

    void submitImageViews(Fleur::Graphics::SFLImageViewInfo* pInfo);

    VkImageView createTextureImageView(VkImage& image, VkFormat format);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkSampler createTextureSampler();

    VkSampler m_ImageSampler;
    std::unordered_map<AssetID, FVkTexture> m_TextureMap;

    void updateStaticGeometryUboDescriptorSets(VkDescriptorSet& set, uint32_t idx, VkImageView imageView, VkSampler& sampler);

    struct SFLDescriptorSetImage
    {
        uint32_t idx;
        VkImageView view;
    };
    std::vector<std::vector<SFLDescriptorSetImage>> m_DescriptorSetImageViewsToUpload;

    uint32_t m_FallbackTextureIdx;
    FVkTexture* m_FallbackCubemapTexture;
    void createFallbackTexture(Fleur::Graphics::SFLImageView& pInfo);

    SFLVertexInput* m_GeometryVertexInput;
    FVkMultisampler* m_MultisampledRenderTarget;

    void createTexture(Fleur::Graphics::SFLImageView& view, FVkTexture& texture, VkFormat format, VkImageAspectFlags aspect, uint32_t mipLevels,
                       uint32_t layerCount);

    FVkTexture* m_DepthRenderTarget;
    void createDepthTexture(FVkTexture& depthRenderTarget, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits sampleCount,
                            uint32_t mipMapCount);

    void startResize();
    void endResize(Fleur::SRect& rect);

    bool m_WindowResizeIsInProgress;

    void BeginRendering(VkCommandBuffer cmd, VkRect2D renderarea, uint32_t currentImage);


    // ---------- static geometry ----------
    FVkDescriptorSetLayout* m_StaticGeometryTexturesDsl;
    FVkDescriptorSetLayout* m_StaticGeometryUboDsl;
    std::vector<VkDescriptorSet> m_StaticGeometryDescriptorSetUbo;
    VkDescriptorSet m_StaticGeometryDescriptorSetTextures;
    VkDescriptorPool m_DescriptorPool;
    std::vector<FVkBuffer> m_UniformBuffers;

    void createStaticGeometryPass();
    void updateUniformBuffer(uint32_t currentImage, Fleur::Graphics::SFLGeometryUBO* pUbo);

    void createDescriptorPool();

    void createDescriptorSets();


    // ---------- skybox ----------
    FVkSkybox* m_Skybox;
    void createSkybox(AssetID id, SFLShaderInfo* pVertexShaderInfo, SFLShaderInfo* pFragmentShaderInfo);
    void setSkybox(AssetID id);

    vk::FVkShader* AddShader(ShaderCreateInfo& shaderInfo);
    std::unordered_map<AssetID, vk::FVkShader> m_ShaderMap;
};
}  // namespace vk