#pragma once

#pragma region Includes& Definitions

#include <Fleur/Log.h>
#include <vulkan/vulkan.h>

#include <Fleur/Math/Math.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "DescriptorPoolAllocator.h"
#include "FVkBuffer.h"
#include "FVkCommand.h"
#include "FVkDebugDraw.h"
#include "FVkDepthTarget.h"
#include "FVkDevice.h"
#include "FVkFloor.h"
#include "FVkMultisampler.h"
#include "FVkOverlayPass.h"
#include "FVkPipeline.h"
#include "FVkPipelineCache.h"
#include "FVkPipelineLayout.h"
#include "FVkShader.h"
#include "FVkSkybox.h"
#include "DirectionalShadowMath.hpp"
#include "FVkSwapchain.h"
#include "FVkTexture.h"
#include "PointLightShadowMap.h"
#include "Renderer_Vulkan.h"
#include "ShadowMapOffsetTexture.h"
#include "VkHelper.h"

#if defined(FL_CONF_DEBUG)
#define DBG_PRINT(moduleText, text)           \
    do                                        \
    {                                         \
        std::ostringstream logMessage;        \
        logMessage << moduleText << text;     \
        FL_CORE_INFO("{}", logMessage.str()); \
    } while (false);
#define MODULE "[Vulkan] "
#define DBG_PRINTM(text)                      \
    do                                        \
    {                                         \
        std::ostringstream logMessage;        \
        logMessage << MODULE << text;         \
        FL_CORE_INFO("{}", logMessage.str()); \
    } while (false);
#else
#define DBG_PRINT(moduleText, text) \
    do                              \
    {                               \
    } while (false)
#define MODULE
#define DBG_PRINTM(text) \
    do                   \
    {                    \
    } while (false)
#endif

#define VULKAN_VERSION VK_API_VERSION_1_4
#define CUBEMAP_LAYERS_COUNT 6

constexpr uint32_t MAX_TEXTURES = 4096;
constexpr uint32_t POINT_LIGHTS_CAPACITY = 12;

#pragma endregion

#pragma region Structs

struct SGPUMaterial
{
    Fleur::Vec4 baseColorFactor{1};
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

    Fleur::Vec3 boundingBoxCenter{};

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
    impl(bool enableValidation, void* pNativeHandle, Fleur::SRect& framebufferSize, Fleur::Graphics::SFLImageView& fallback, uint32_t maxPointLights,
         uint32_t cascadeCount, Fleur::Graphics::LightSampling directionalLight, Fleur::Graphics::LightSampling pointLight,
         std::shared_ptr<spdlog::logger> logger);
    ~impl();

    std::shared_ptr<spdlog::logger> m_Logger;

    bool beginFrame(const Fleur::Graphics::RenderFrameData& frameData);
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

    std::unique_ptr<FVkDevice> m_Device;
    std::chrono::steady_clock::time_point m_LastMemoryDiagnosis{};
    std::unique_ptr<FVkSwapchain> m_Swapchain;
    VkSurfaceKHR m_Surface;
    VkSurfaceKHR createSurface(VkInstance instance, void* nativeHandle);
    std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME};

    // ---------- opaque pipelines ----------

    FVkPipeline* m_OpaquePipeline{nullptr};
    FVkPipeline* createOpaquePipeline(Fleur::Graphics::SFLShaderBytecode pVertexInfo, Fleur::Graphics::SFLShaderBytecode pFragmentInfo,
                                      VkSampleCountFlagBits samplesCount);
    FVkPipeline* m_TransparentPipeline{nullptr};
    FVkPipeline* createTransparentPipeline(Fleur::Graphics::SFLShaderBytecode pVertexInfo, Fleur::Graphics::SFLShaderBytecode pFragmentInfo,
                                           VkSampleCountFlagBits samplesCount);

    FVkPipeline* m_ShadowPipeline{nullptr};
    FVkPipeline* createShadowPipeline(Fleur::Graphics::SFLShaderBytecode pVertexInfo, Fleur::Graphics::SFLShaderBytecode pGeometryInfo,
                                      Fleur::Graphics::SFLShaderBytecode pFragmentInfo, VkSampleCountFlagBits samplesCount);
    FVkPipeline* createGraphicsPipeline(const char* shaderKey, Fleur::Graphics::SFLShaderBytecode pVertexInfo,
                                        Fleur::Graphics::SFLShaderBytecode pFragmentInfo, const vk::GetPipelineInfo& pipelineInfo,
                                        Fleur::Graphics::SFLShaderBytecode pGeometryInfo = {});

    // ---------- shaders ----------
    VkShaderModule createShaderModule(Fleur::Graphics::SFLShaderBytecode* pShaderInfo);


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
        VkDescriptorSet m_ShadowMapDescriptorSet{VK_NULL_HANDLE};
        VkDescriptorSet m_DirectionalShadowMatricesDescriptor{VK_NULL_HANDLE};
        FVkBuffer m_DirectionalShadowMatricesBuffer;

        FVkBuffer m_CameraBuffer;
        VkDescriptorSet m_CameraDescriptor{VK_NULL_HANDLE};

        FVkBuffer m_SceneNodeTransformsStorageBuffer;
        VkDescriptorSet m_SceneNodeTransformsDescriptor{VK_NULL_HANDLE};
    };

    PointLightShadowMap m_PointLightShadowMaps;
    VkDescriptorSet m_PointLightsDescriptorSet{VK_NULL_HANDLE};
    VkDescriptorSet m_PointLightShadowMapsDescriptorSet{VK_NULL_HANDLE};
    VkDescriptorSet m_ShadowMapOffsetDescriptorSet{VK_NULL_HANDLE};
    VkDescriptorSet m_OverlayShadowMapDescriptorSet{VK_NULL_HANDLE};

    std::shared_ptr<FVkPipelineLayout> m_OpaquePipelineLayout;
    std::shared_ptr<FVkPipelineLayout> m_ShadowPipelineLayout;
    bool m_OpaqueDescriptorSetsInitialized{false};
    FVkPipelineCache m_PipelineCache;
    std::unordered_map<std::string, std::shared_ptr<FVkPipelineLayout>> m_PipelineLayouts;
    struct Frame
    {
        FVkCommandPool m_CommandPools;
        FVkCommandBuffer m_CommandBuffers;
        VkFence m_InFlightFences{VK_NULL_HANDLE};
        VkSemaphore m_ImagesAvailable{VK_NULL_HANDLE};

        FVkDepthTarget m_DirectionalLightShadowMap;
        VkImageLayout m_DirectionalLightShadowMapLayout{VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        vk::abstraction::DescriptorAllocator frameDescriptors;
        FrameSceneResources scene;
    };

    Frame& GetCurrentFrame()
    {
        return m_Frames[m_CurrentFrame];
    }
    std::vector<Frame> m_Frames;
    std::vector<VkSemaphore> m_RenderFinished;
    std::unique_ptr<FVkCommandPool> m_ImmediateCommandPool;

    const uint32_t K_NODE_TRANSFORMS_CUP = 1023;

    VkMemoryRequirements memRequirements;


    std::unique_ptr<FVkBuffer> m_VertexBuffer;
    std::unique_ptr<FVkBuffer> m_IndexBuffer;

    std::vector<Fleur::Mat4> m_InstanceNodeTransforms;

    std::unique_ptr<FVkBuffer> m_PointLightsBuffer;
    // VkDescriptorSet m_PointLightDescriptorSet;
    uint32_t m_MaxPointLights{0};

    std::vector<InstancesBatch> m_Batches;
    std::vector<PrimitiveDrawInfo> m_Primitives;
    std::vector<InstanceDrawInfo> m_Instances;

    struct FLFrameDrawItem
    {
        uint32_t primitiveIdx{};

        uint32_t instanceCount{};

        uint32_t modelTransformIdx{};
        uint32_t nodeTransformsStartIdx{};

        Fleur::Vec3 boundingBoxCenter{};
    };

    std::vector<FLFrameDrawItem> m_OpaqueDrawItems;
    std::vector<FLFrameDrawItem> m_TransparentDrawItems;

    std::unordered_map<AssetID, uint32_t> m_RegisteredModels;

    void registerModel(AssetID id, const SVertexData* vertices, uint32_t verticesCount, const uint32_t* indices, uint32_t indexCount,
                       const Fleur::Mat4* transformNodes, uint32_t transformNodesCount, const FLPrimitiveDrawItem* primitives, uint32_t primitiveCount,
                       const FLInstanceItem* srcInstances, uint32_t instanceCount);
    void unregisterModel(AssetID id);
    void drawModel(AssetID id, const Fleur::Mat4& transform);

    uint32_t m_CurrentFrame{0};
    static constexpr uint32_t kFramesInFlight = 3;
    uint32_t m_FramesInFlight{kFramesInFlight};
    uint32_t m_ImageIndex = 0;
    std::vector<VkImageLayout> m_SwapchainImageLayouts;

    void uploadTextures(Fleur::Graphics::SFLImageViewInfo* pInfo);

    VkImageView createTextureImageView(VkImage& image, VkFormat format);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkSampler createTextureSampler();
    VkSampler createShadowMapSampler();
    void createRenderFinishedSemaphores();
    void destroyRenderFinishedSemaphores();

    VkSampler m_ImageSampler;
    std::unordered_map<AssetID, FVkTexture> m_TextureMap;

    void updateTextureDescriptorSet(VkDescriptorSet& set, uint32_t idx, VkImageView imageView, VkSampler& sampler);

    struct SFLDescriptorSetImage
    {
        uint32_t idx;
        VkImageView view;
    };
    std::vector<std::vector<SFLDescriptorSetImage>> m_DescriptorSetImageViewsToUpload;

    uint32_t m_FallbackTextureIdx;
    std::unique_ptr<FVkTexture> m_FallbackCubemapTexture;
    void createFallbackTexture(Fleur::Graphics::SFLImageView& pInfo);

    std::unique_ptr<FVkMultisampler> m_MultisampledRenderTarget;

    void createTexture(Fleur::Graphics::SFLImageView& view, FVkTexture& texture, VkFormat format, VkImageAspectFlags aspect, uint32_t mipLevels,
                       uint32_t layerCount);

    FVkDepthTarget m_DepthRenderTarget;
    VkSampler m_ShadowMapSampler{VK_NULL_HANDLE};
    void updateShadowMapDescriptorSets();
    ShadowMapOffsetTexture m_ShadowMapOffsetTexture;

    struct ShadowMapFrustumSettings
    {
        Fleur::Vec3 center{0.0f, 0.0f, 0.0f};
        float halfSize{50.0f};
        float nearDistanceFactor{0.3f};
        float farExtension{100.0f};
    };
    ShadowMapFrustumSettings m_ShadowMapFrustumSettings;

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
    void BeginShadowRendering();
    void ExecuteShadowPass();
    void UpdateDirectionalShadowFrustum(const Fleur::Vec3& lightDirection);
    void ExecuteDirectionalShadowSubpass();
    void ExecutePointLightShadowSubpass();
    void ExecuteMainPass();
    void SubmitFrame();

    struct DirectionalLightShadowFrustum
    {
        Fleur::Mat4 lightSpaceMatrix{};

        Fleur::Vec3 center{0.0f};
        float radius{0.0f};
    };

    DirectionalLightShadowFrustum BuildDirectionalShadowFrustum(const std::array<Fleur::Vec4, 8>& corners, const Fleur::Vec3& lightDirection,
                                                                const std::array<Fleur::Vec4, 8>* casterCorners = nullptr,
                                                                size_t cascadeIndex = std::numeric_limits<size_t>::max()) const;
    static constexpr size_t kMaxCascadeCount = 16;

    struct DirectionalShadowMatrices
    {
        std::array<Fleur::Mat4, kMaxCascadeCount> lightSpaceMatrices{};
        uint32_t cascadeCount = 0;
        uint32_t padding[3]{};
        std::array<Fleur::Vec4, 4> cascadeSplits{};
    };

    static_assert(sizeof(DirectionalShadowMatrices) == 1104);
    static_assert(offsetof(DirectionalShadowMatrices, lightSpaceMatrices) == 0);
    static_assert(offsetof(DirectionalShadowMatrices, cascadeCount) == 1024);
    static_assert(offsetof(DirectionalShadowMatrices, cascadeSplits) == 1040);

    std::array<DirectionalLightShadowFrustum, kMaxCascadeCount> m_CascadeShadowFrustums{};
    mutable std::array<float, kMaxCascadeCount> m_StableCascadeHalfWidths{};
    mutable std::array<float, kMaxCascadeCount> m_StableCascadeHalfHeights{};

    // ---------- textures ----------
    VkDescriptorSet m_TextureDescriptorSet{VK_NULL_HANDLE};
    VkDescriptorPool m_DescriptorPool{VK_NULL_HANDLE};

    void initializeOpaqueDescriptorSets();

    void createTextureDescriptorPool();

    void createTextureDescriptorSets();


    // ---------- skybox ----------
    std::unique_ptr<FVkSkybox> m_Skybox;
    std::unique_ptr<FVkFloor> m_Floor;
    const Fleur::Graphics::ShaderRegistry* m_ShaderRegistry{nullptr};
    Fleur::Graphics::BoundingBox m_ShadowSceneBounds{};
    bool m_HasShadowSceneBounds{false};
    Fleur::Vec3 m_LastDirectionalLightDirection{0.0f};
    bool m_HasLastDirectionalLightDirection{false};
    uint32_t m_CascadeCount{5};
    Fleur::Graphics::LightSampling m_DirectionalLightSampling{Fleur::Graphics::LightSampling::Default};
    Fleur::Graphics::LightSampling m_PointLightSampling{Fleur::Graphics::LightSampling::Default};
    void setShaderRegistry(const Fleur::Graphics::ShaderRegistry& shaders)
    {
        m_ShaderRegistry = &shaders;
    }
    void setShadowSceneBounds(const Fleur::Graphics::BoundingBox& bounds)
    {
        m_ShadowSceneBounds = bounds;
        m_HasShadowSceneBounds = true;
        m_StableCascadeHalfWidths.fill(0.0f);
        m_StableCascadeHalfHeights.fill(0.0f);
    }
    void setShadowSettings(uint32_t cascadeCount, Fleur::Graphics::LightSampling directionalLight,
                           Fleur::Graphics::LightSampling pointLight)
    {
        m_CascadeCount = std::clamp(cascadeCount, 1u, static_cast<uint32_t>(kMaxCascadeCount));
        m_DirectionalLightSampling = directionalLight;
        m_PointLightSampling = pointLight;
    }
    Fleur::Graphics::SFLShaderBytecode shaderInfo(std::string_view name) const;
    void createSkybox(AssetID id, SFLShaderStages shaderStages);
    void setSkybox(AssetID id);
    void createFloor(AssetID texture, SFLShaderStages shaderStages, float height);
    void setFloor(AssetID texture, float height);

    std::unordered_map<std::string, vk::FVkShader> m_ShaderMap;

    void createPass(EFLPassKind kind, SFLShaderStages shaderStages);
    void createShadowPass(EFLShadowPassKind kind, SFLShaderStages shaderStages);


    // Debug
    std::unique_ptr<FVkDebugDraw> m_DebugDraw;
    std::unique_ptr<FVkOverlayPass> m_OverlayPass;

    std::vector<SFLPointLight> m_PointLights;
    void updatePointLight(const SFLPointLight* light, uint32_t lightCount);

    // Descriptors
    Fleur::Graphics::RenderFrameData m_FrameData;

    bool m_FloorTextureWasLoaded{false};
    int32_t m_FloorTextureIdx{-1};

    // Cascade shadow mapping
    template <size_t N>
    struct CascadeSplits
    {
        std::array<float, N> splits;
        static constexpr float step = 1.0f / N;
        CascadeSplits() : splits{}
        {
            Update(0.1f, 1000.0f, 1.0f, N);
        }

        void Update(float nearPlane, float farPlane, float lambda, size_t activeCount)
        {
            const float safeNear = std::max(nearPlane, 0.001f);
            const float safeFar = std::max(farPlane, safeNear + 0.001f);
            activeCount = std::clamp(activeCount, size_t{1}, N);
            splits.fill(1.0f);

            // Keep the first cascade useful for more than the tiny region
            // immediately in front of the camera. With a logarithmic split
            // and a far plane of 1000, the first split would otherwise be
            // below one world unit.
            constexpr float minimumFirstCascadeFar = 10.0f;
            const float firstCascadeFar = std::min(safeFar, std::max(safeNear, minimumFirstCascadeFar));

            for (size_t i = 0; i < activeCount; ++i)
            {
                float split = firstCascadeFar;
                if (activeCount == 1)
                    split = safeFar;
                if (activeCount > 1)
                {
                    if (i > 0)
                    {
                        const float p = static_cast<float>(i) / static_cast<float>(activeCount - 1);
                        const float logarithmic = firstCascadeFar * std::pow(safeFar / firstCascadeFar, p);
                        const float uniform = firstCascadeFar + (safeFar - firstCascadeFar) * p;
                        split = uniform * (1.0f - lambda) + logarithmic * lambda;
                    }
                }
                splits[i] = (split - safeNear) / (safeFar - safeNear);
            }
        }
    };
    CascadeSplits<kMaxCascadeCount> m_CascadeSplits;

    std::vector<Fleur::Vec4> getFrustumCornersWorldSpace(const Fleur::Mat4& proj, const Fleur::Mat4& view);
    std::array<Fleur::Vec4, 8> SplitFrustum(const std::vector<Fleur::Vec4>& corners, float splitNear, float splitFar);
};
}  // namespace vk
