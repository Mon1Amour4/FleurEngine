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

    void update(Fleur::Graphics::SFLGeometryUBO* pUbo);

    void set_skybox(AssetID id);

    // Instance
    VkInstance m_VulkanInstance;
    bool m_ValidationsEnabled{false};
    VkInstance create_instance(bool enableValidation, const std::vector<const char*>& instanceExtensions, const std::vector<const char*>& validationLayers);

    VkDebugUtilsMessengerEXT debugMessenger;
    void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setup_debug_messenger();
    VkResult create_debug_utils_messenger_EXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                              const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void destroy_debug_utils_messenger_EXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

    Fleur::SRect m_SurfaceRect;

    FVkDevice* m_Device;
    FVkSwapchain* m_Swapchain;
    VkSurfaceKHR m_Surface;
    VkSurfaceKHR create_surface(VkInstance instance, void* nativeHandle);
    std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // ---------- geometryPipeline ----------
    VkDescriptorSetLayout m_GeometryDSL;
    FVkPipeline* m_GeometryPipeline;
    FVkPipeline* create_geometry_pipeline(Fleur::Graphics::SFLShaderInfo* pVertexInfo, Fleur::Graphics::SFLShaderInfo* pFragmentInfo,
                                          Fleur::Graphics::EFLInputAssemblyTopology pInputAssemblyTopology, VkSampleCountFlagBits samplesCount);

    // ---------- renderpass ----------
    VkRenderPass m_GeometryRenderPass;
    void create_geometry_renderpass();


    // ---------- shaders ----------
    VkShaderModule create_shader_module(Fleur::Graphics::SFLShaderInfo* pShaderInfo);


    // ---------- commandPool ----------
    FVkCommandPool* m_GraphicsCommandPool;


    // ---------- commandBuffer ----------
    struct SFLCmdBuffer
    {
        std::vector<VkCommandBuffer> buffers;
        std::vector<bool> validation;
        void invalidate();
        bool are_valid();
    };

    std::vector<FVkCommandBuffer> m_PrimaryCmdBuffers;
    FVkCommandBuffer m_SkyboxCmd;

    std::vector<FVkCommandBuffer> m_SecondaryCmdBuffers;
    std::vector<bool> m_SecondaryCmdValidation;
    void init_geometry_primary_cmd_buffers(uint32_t idx);
    void update_geometry_secondary_cmd_buffer(uint32_t idx);


    // ---------- synchronization ----------
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    void create_sync_objects();

    std::vector<FVkBuffer> m_UniformBuffers;

    VkMemoryRequirements memRequirements;

    void create_uniform_buffers();
    void update_uniform_buffer(uint32_t currentImage, Fleur::Graphics::SFLGeometryUBO* pUbo);

    void create_descriptor_set_layout();

    VkDescriptorPool descriptorPool;
    void create_descriptor_pool();

    std::vector<VkDescriptorSet> descriptorSets;
    void create_descriptor_sets();


    // ---------- vma ----------
    VmaAllocator m_Allocator;
    void initialize_Vma();
    void free_Vma();

    FVkBuffer* m_VertexBuffer;
    FVkBuffer* m_IndexBuffer;


    std::vector<DrawInfo> m_DrawList;
    void add_to_draw_list(Fleur::Graphics::SFLModelView* pModelView);

    VkVertexInputBindingDescription GetVertexDataBindingDescriptor();
    std::array<VkVertexInputAttributeDescription, 3> GetVertexDataAttributeDescriptions();

    uint32_t currentFrame = 0;

    void submit_image_views(Fleur::Graphics::SFLImageViewInfo* pInfo);

    VkImageView create_texture_image_view(VkImage& image, VkFormat format);
    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkSampler create_texture_sampler();

    VkSampler m_ImageSampler;
    std::unordered_map<AssetID, FVkTexture> m_TextureMap;
    uint32_t m_FallbackTextureIdx;
    void update_descriptor_sets(VkDescriptorSet& set, uint32_t idx, VkImageView imageView, VkSampler& sampler);

    struct SFLDescriptorSetImage
    {
        uint32_t idx;
        VkImageView view;
    };
    std::vector<std::vector<SFLDescriptorSetImage>> m_DescriptorSetImageViews;

    FVkTexture* m_FallbackTexture;
    void create_fallback_texture(Fleur::Graphics::SFLImageView& pInfo);


    // ---------- depth ----------
    struct Depth
    {
        Depth() = default;
        FVkTexture* depthTexture;
    };
    void create_depth_buffer(vk::backend::impl::Depth& depthBuffer, VkPhysicalDevice device, VkSampleCountFlagBits samplesCount, uint32_t mimLevels);
    Depth m_Depth;

    SFLVertexInput* m_GeometryVertexInput;
    FVkMultisampler* m_Multisampler;

    void create_texture(FVkTexture& texture, Fleur::Graphics::SFLImageView& view, VkFormat format, VkImageAspectFlags aspect, uint32_t mipLevels);

    void create_depth_texture(FVkTexture& texture, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits samplesCount, uint32_t mimLevels);

    void start_resize();
    void end_resize(Fleur::SRect& rect);

    bool m_WindowResizeIsInProgress;

    FVkSkybox* m_Skybox;
    void create_skybox(AssetID id, SFLShaderInfo* pVertexShaderInfo, SFLShaderInfo* pFragmentShaderInfo);
};
}  // namespace vk