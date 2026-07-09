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
#include <sstream>
#include <vector>

#include "DescriptorPoolAllocator.h"
#include "FVkBuffer.h"
#include "FVkCommand.h"
#include "FVkDebugDraw.h"
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
    glm::vec4 baseColorFactor{1};
    uint32_t albedo{0};
    uint32_t normal{0};
    float metallic{1};
    float roughness{1};
    float alphaCutoff{0};
};
struct InstancesBatch
{
    uint32_t instancesCount{};
    uint32_t instanceStartIdx{};

    uint32_t globalNodeStartIdx{};
    uint32_t nodeTransformCount{};
};
struct InstanceDrawInfo
{
    uint32_t drawCount;
    uint32_t globalNodeTransformStartIdx;

    uint32_t primitiveCount;
    uint32_t globalPrimitiveStartIdx;
};

struct PrimitiveDrawInfo
{
    FLAlphaMode bucket{FLAlphaMode::FL_OPAQUE};

    uint64_t indexCount{0};
    uint64_t vertexCount{0};

    uint64_t indexOffset{0};
    uint64_t vertexOffset{0};

    SGPUMaterial material;

    glm::vec3 boundingBoxCenter{};

    void FromMaterial(const Fleur::Graphics::FLMaterial& mat)
    {
        material.albedo = mat.albedo;

        material.alphaCutoff = mat.alphaCutoff;
        material.baseColorFactor = mat.baseColorFactor;

        material.normal = mat.normal;
    }
};

SFLPushConstant MakePush(const PrimitiveDrawInfo& info)
{
    SFLPushConstant pc{};
    pc.indices.z = info.material.albedo;
    pc.baseColorFactor = {info.material.baseColorFactor.r, info.material.baseColorFactor.g, info.material.baseColorFactor.b, info.material.baseColorFactor.a};
    pc.materialParams.x = info.material.alphaCutoff;

    return pc;
}
#pragma endregion

// ---------- static function ----------
static const char* SeverityToString(VkDebugUtilsMessageSeverityFlagBitsEXT severity)
{
    switch (severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        return "VERBOSE";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        return "INFO";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        return "WARNING";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

static std::string MessageTypeToString(VkDebugUtilsMessageTypeFlagsEXT type)
{
    std::string result;

    if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
        result += "GENERAL ";

    if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        result += "VALIDATION ";

    if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        result += "PERFORMANCE ";

    if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
        result += "DEVICE_ADDRESS_BINDING ";

    return result.empty() ? "UNKNOWN" : result;
}

static const char* ObjectTypeToString(VkObjectType type)
{
    switch (type)
    {
    case VK_OBJECT_TYPE_INSTANCE:
        return "INSTANCE";
    case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
        return "PHYSICAL_DEVICE";
    case VK_OBJECT_TYPE_DEVICE:
        return "DEVICE";
    case VK_OBJECT_TYPE_QUEUE:
        return "QUEUE";
    case VK_OBJECT_TYPE_SEMAPHORE:
        return "SEMAPHORE";
    case VK_OBJECT_TYPE_COMMAND_BUFFER:
        return "COMMAND_BUFFER";
    case VK_OBJECT_TYPE_FENCE:
        return "FENCE";
    case VK_OBJECT_TYPE_DEVICE_MEMORY:
        return "DEVICE_MEMORY";
    case VK_OBJECT_TYPE_BUFFER:
        return "BUFFER";
    case VK_OBJECT_TYPE_IMAGE:
        return "IMAGE";
    case VK_OBJECT_TYPE_EVENT:
        return "EVENT";
    case VK_OBJECT_TYPE_QUERY_POOL:
        return "QUERY_POOL";
    case VK_OBJECT_TYPE_BUFFER_VIEW:
        return "BUFFER_VIEW";
    case VK_OBJECT_TYPE_IMAGE_VIEW:
        return "IMAGE_VIEW";
    case VK_OBJECT_TYPE_SHADER_MODULE:
        return "SHADER_MODULE";
    case VK_OBJECT_TYPE_PIPELINE_CACHE:
        return "PIPELINE_CACHE";
    case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
        return "PIPELINE_LAYOUT";
    case VK_OBJECT_TYPE_RENDER_PASS:
        return "RENDER_PASS";
    case VK_OBJECT_TYPE_PIPELINE:
        return "PIPELINE";
    case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
        return "DESCRIPTOR_SET_LAYOUT";
    case VK_OBJECT_TYPE_SAMPLER:
        return "SAMPLER";
    case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
        return "DESCRIPTOR_POOL";
    case VK_OBJECT_TYPE_DESCRIPTOR_SET:
        return "DESCRIPTOR_SET";
    case VK_OBJECT_TYPE_FRAMEBUFFER:
        return "FRAMEBUFFER";
    case VK_OBJECT_TYPE_COMMAND_POOL:
        return "COMMAND_POOL";
    case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
        return "SWAPCHAIN_KHR";
    default:
        return "UNKNOWN";
    }
}
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    if (messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        return VK_FALSE;

    std::ostringstream oss;

    oss << "\n[Vulkan][" << SeverityToString(messageSeverity) << "] "
        << "[" << MessageTypeToString(messageType) << "]\n";

    if (pCallbackData->pMessageIdName)
        oss << "MessageIdName: " << pCallbackData->pMessageIdName << "\n";

    oss << "MessageIdNumber: " << pCallbackData->messageIdNumber << "\n";

    if (pCallbackData->pMessage)
        oss << "Message: " << pCallbackData->pMessage << "\n";

    if (pCallbackData->objectCount > 0)
    {
        oss << "Objects:\n";

        for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
        {
            const auto& object = pCallbackData->pObjects[i];

            oss << "  [" << i << "] "
                << "type=" << ObjectTypeToString(object.objectType) << ", handle=0x" << std::hex << object.objectHandle << std::dec;

            if (object.pObjectName)
                oss << ", name=" << object.pObjectName;
            else
                oss << ", name=<unnamed>";

            oss << "\n";
        }
    }

    if (pCallbackData->queueLabelCount > 0)
    {
        oss << "Queue labels:\n";

        for (uint32_t i = 0; i < pCallbackData->queueLabelCount; ++i)
        {
            oss << "  [" << i << "] " << pCallbackData->pQueueLabels[i].pLabelName << "\n";
        }
    }

    if (pCallbackData->cmdBufLabelCount > 0)
    {
        oss << "Command buffer labels:\n";

        for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; ++i)
        {
            oss << "  [" << i << "] " << pCallbackData->pCmdBufLabels[i].pLabelName << "\n";
        }
    }

    DBG_PRINTM(oss.str());

    return VK_FALSE;
}

namespace vk
{
PFN_vkCmdBeginDebugUtilsLabelEXT myVkCmdBeginDebugUtilsLabelEXT = nullptr;
PFN_vkCmdEndDebugUtilsLabelEXT myVkCmdEndDebugUtilsLabelEXT = nullptr;
PFN_vkSetDebugUtilsObjectNameEXT SetDebugUtilsObjectNameEXT = nullptr;

struct backend::impl
{
    impl(bool enableValidation, void* pNativeHandle, Fleur::SRect& framebufferSize, Fleur::Graphics::SFLImageView& fallback);
    ~impl();

    bool beginFrame(Fleur::Graphics::SFLCameraData& cameraData);
    void endFrame();

    // MeshInstance
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

    FVkPipeline* m_GeometryPipeline{nullptr};
    FVkPipeline* createGeometryPipeline(Fleur::Graphics::SFLShaderInfo pVertexInfo, Fleur::Graphics::SFLShaderInfo pFragmentInfo,
                                        VkSampleCountFlagBits samplesCount, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    FVkPipeline* m_TransparentPipeline{nullptr};
    FVkPipeline* createTransparentPipeline(Fleur::Graphics::SFLShaderInfo pVertexInfo, Fleur::Graphics::SFLShaderInfo pFragmentInfo,
                                           VkSampleCountFlagBits samplesCount, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);

    FVkPipeline* m_ShadowPipeline{nullptr};
    FVkPipeline* createShadowPipeline(Fleur::Graphics::SFLShaderInfo pVertexInfo, Fleur::Graphics::SFLShaderInfo pFragmentInfo,
                                      VkSampleCountFlagBits samplesCount, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    FVkPipeline* createGraphicsPipeline(const char* shaderKey, Fleur::Graphics::SFLShaderInfo pVertexInfo, Fleur::Graphics::SFLShaderInfo pFragmentInfo,
                                        const vk::GetPipelineInfo& pipelineInfo, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);

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

    struct FrameSceneResources
    {
        FVkBuffer m_CameraBuffer;
        VkDescriptorSet m_CameraDescriptor{VK_NULL_HANDLE};

        FVkBuffer m_SceneNodeTransformsStorageBuffer;
        VkDescriptorSet m_SceneNodeTransformsDescriptor{VK_NULL_HANDLE};
    };

    FVkDescriptorSetLayout* m_CameraUboLayout{nullptr};
    FVkDescriptorSetLayout* m_SceneNodeTransformsLayout{nullptr};
    FVkDescriptorSetLayout* m_ShadowMapLayout{nullptr};
    FVkDescriptorSetLayout* m_PointLightsLayout{nullptr};
    FVkDescriptorSetLayout* m_StaticGeometryTexturesLayout{nullptr};
    struct Frame
    {
        FVkCommandPool m_CommandPools;
        FVkCommandBuffer m_CommandBuffers;
        VkFence m_InFlightFences{VK_NULL_HANDLE};
        VkSemaphore m_ImagesAvailable{VK_NULL_HANDLE};
        VkSemaphore m_RenderFinished{VK_NULL_HANDLE};

        vk::abstraction::DescriptorAllocator frameDescriptors;
        FrameSceneResources scene;
    };

    Frame& GetCurrentFrame()
    {
        return m_Frames[m_CurrentFrame];
    }
    std::vector<Frame> m_Frames;
    FVkCommandPool* m_ImmediateCommandPool{nullptr};

    const uint32_t K_NODE_TRANSFORMS_CUP = 1023;

    VkMemoryRequirements memRequirements;


    // ---------- vma ----------
    VmaAllocator m_Allocator;
    void initializeVma();
    void freeVma();

    FVkBuffer* m_VertexBuffer{nullptr};
    FVkBuffer* m_IndexBuffer{nullptr};

    VkDescriptorSet m_ShadowMapDescriptorSet;
    void InitShadowMapDescriptorSet();

    std::vector<glm::mat4> m_InstanceNodeTransforms;

    FVkBuffer* m_PointLightsBuffer{nullptr};
    VkDescriptorSet m_PointLightDescriptorSet;

    std::vector<InstancesBatch> m_Batches;
    std::vector<PrimitiveDrawInfo> m_Primitives;
    std::vector<InstanceDrawInfo> m_Instances;

    struct FLFrameDrawItem
    {
        uint32_t primitiveIdx{};

        uint32_t instanceCount{};

        uint32_t modelTransformIdx{};
        uint32_t nodeTransformsStartIdx{};

        glm::vec3 boundingBoxCenter{};
    };

    std::vector<FLFrameDrawItem> m_OpaqueDrawItems;
    std::vector<FLFrameDrawItem> m_TransparentDrawItems;

    std::unordered_map<AssetID, uint32_t> m_RegisteredModels;

    void registerModel(AssetID id, const SVertexData* vertices, uint32_t verticesCount, const uint32_t* indices, uint32_t indexCount,
                       const glm::mat4* transformNodes, uint32_t transformNodesCount, const FLPrimitiveDrawItem* primitives, uint32_t primitiveCount,
                       const FLInstanceItem* srcInstances, uint32_t instanceCount);
    void unregisterModel(AssetID id);
    void drawModel(AssetID id, const glm::mat4& transform);

    uint32_t m_CurrentFrame{0};
    uint32_t m_FramesInFlight{0};
    uint32_t m_ImageIndex = 0;

    // Per-frame camera (set in beginFrame, consumed by skybox/debug in endFrame).
    Fleur::Graphics::SFLCameraData m_CameraData;

    void uploadTextures(Fleur::Graphics::SFLImageViewInfo* pInfo);

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
    FVkTexture* m_FallbackCubemapTexture{nullptr};
    void createFallbackTexture(Fleur::Graphics::SFLImageView& pInfo);

    FVkMultisampler* m_MultisampledRenderTarget;

    void createTexture(Fleur::Graphics::SFLImageView& view, FVkTexture& texture, VkFormat format, VkImageAspectFlags aspect, uint32_t mipLevels,
                       uint32_t layerCount);

    FVkTexture* m_DepthRenderTarget{nullptr};
    void createDepthTexture(FVkTexture& depthRenderTarget, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits sampleCount,
                            uint32_t mipMapCount);

    FVkTexture* m_ShadowMapRenderTarget{nullptr};
    void createShadowMapTexture(FVkTexture& depthRenderTarget, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits sampleCount,
                                uint32_t mipMapCount);

    void startResize();
    void endResize(Fleur::SRect& rect);

    bool m_WindowResizeIsInProgress{false};

    struct FRenderingColorAttachmentDesc
    {
        VkImageView imageView = VK_NULL_HANDLE;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkClearValue clearValue{.color = {{1.0f, 1.0f, 1.0f, 1.0f}}};

        VkResolveModeFlagBits resolveMode = VK_RESOLVE_MODE_NONE;
        VkImageView resolveImageView = VK_NULL_HANDLE;
        VkImageLayout resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    struct FRenderingDepthAttachmentDesc
    {
        VkImageView imageView = VK_NULL_HANDLE;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkClearValue clearValue{.depthStencil = {.depth = 1.0f, .stencil = 0}};
    };

    struct FBeginRenderingDesc
    {
        VkRect2D renderArea{};

        const FRenderingColorAttachmentDesc* colorAttachment = nullptr;
        const FRenderingDepthAttachmentDesc* depthAttachment = nullptr;

        uint32_t layerCount = 1;
        uint32_t viewMask = 0;
    };
    void BeginRendering(VkCommandBuffer cmd, const FBeginRenderingDesc& desc);
    void ExecuteShadowPass();

    void ExecuteMainPass();


    // ---------- static geometry ----------
    VkDescriptorSet m_StaticGeometryDescriptorSetTextures;
    VkDescriptorPool m_DescriptorPool;

    void createStaticGeometryPass();

    void createDescriptorPool();

    void createDescriptorSets();


    // ---------- skybox ----------
    FVkSkybox* m_Skybox{nullptr};
    void createSkybox(AssetID id, SFLShaderStages shaderStages);
    void setSkybox(AssetID id);

    vk::FVkShader* AddShader(ShaderCreateInfo& shaderInfo);
    std::unordered_map<std::string, vk::FVkShader> m_ShaderMap;

    void createPass(EFLPassKind kind, SFLShaderStages shaderStages);


    // Debug
    FVkDebugDraw* m_DebugDraw{nullptr};

    // directionIntensity - first 3 are direction and fourth is intensity
    // intensity [0;1]
    struct FVkDirectionalLight
    {
        glm::vec4 directionIntensity{glm::vec4(1, 0, 0, 1)};
        glm::vec4 color{glm::vec4(1, 1, 1, 1)};
    };
    FVkDirectionalLight m_DirectionalLight;
    void setDirectionalLight(glm::vec3 direction, glm::vec4 color, float intensity);

    std::vector<SFLPointLight> m_PointLights;
    void updatePointLight(const SFLPointLight* light, uint32_t lightCount);

    // Descriptors
    vk::abstraction::DescriptorAllocator m_DescriptorAlloc;
};
}  // namespace vk
