#pragma once

#include <vulkan/vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include <glm/mat4x4.hpp>
#include <vector>

struct SFLPushConstant
{
    glm::mat4 lightSpaceMatrix;
    glm::vec4 baseColorFactor;

    // x = nodeTransformsStartIdx
    // y = modelTransformIdx
    // z = materialIndex
    // w = pointLightCount
    glm::uvec4 indices;

    // x = alphaCutoff
    // yzw = unused
    glm::vec4 materialParams;

    glm::vec4 directionalLightColor;

    // xyz = direction, w = intensity
    glm::vec4 directionalLightDirectionIntensity;

    // xyz = camera position, w = unused
    glm::vec4 cameraPos;
};

class FVkCommandPool
{
public:
    FVkCommandPool();
    ~FVkCommandPool();

    void Init(VkDevice device, VkCommandPoolCreateFlagBits usage, uint32_t queueFamilyIndex);

    inline VkCommandPool GetCommandPool() const
    {
        return m_CommandPool;
    }

private:
    VkCommandPool m_CommandPool;
    VkDevice m_Device;
    VkCommandPoolCreateFlagBits m_Usage;
    int m_QueueFamilyIndex;
};

class FVkCommandBuffer
{
public:
    FVkCommandBuffer();
    ~FVkCommandBuffer();

    void Init(VkDevice device, VkCommandPool pool, VkCommandBufferLevel level);

    inline bool Valid() const
    {
        return m_Valid;
    }
    void Reset();
    void Begin();
    void End();

    void BindPipeline(VkPipeline pipeline);
    void ExecuteSecondaryCommandBuffer(VkCommandBuffer secondary);

    void SetViewport(VkViewport viewport);
    void SetScissors(VkRect2D scissors);

    void BindVertexBuffer(VkBuffer* buffer);
    void BindIndexBuffer(VkBuffer* buffer, VkIndexType indextype);
    void BindDescriptorSets(VkPipelineLayout pipelineLayout, VkDescriptorSet* descriptorSets, uint32_t descriptorSetsCount);

    void CmdBeginDebugLabel(PFN_vkCmdBeginDebugUtilsLabelEXT func, const char* name);
    void CmdEndDebugLabel(PFN_vkCmdEndDebugUtilsLabelEXT func);

    template <typename VulkanHandle>
    VkResult CmdSetObjectName(PFN_vkSetDebugUtilsObjectNameEXT func, VkDevice device, VkObjectType objectType, VulkanHandle handle, const char* name)
    {
        if (func == nullptr || name == nullptr)
            return VK_SUCCESS;

        uint64_t handleValue = 0;

        if constexpr (std::is_pointer_v<VulkanHandle>)
        {
            handleValue = reinterpret_cast<uint64_t>(handle);
        }
        else
        {
            handleValue = static_cast<uint64_t>(handle);
        }

        return SetObjectName(func, device, objectType, handleValue, name);
    }


    template <typename T>
    void PushConstant(VkPipelineLayout pipelineLayout, VkShaderStageFlags shaderStage, T constant)
    {
        vkCmdPushConstants(m_CommandBuffer, pipelineLayout, shaderStage, 0, sizeof(T), &constant);
    }

    void Draw(uint32_t count, uint32_t vertexStart = 0);
    void DrawIndexed(uint32_t indexCount, size_t indexOffset, size_t vertexOffset, uint32_t instanceCout, uint32_t instanceStartIdx);

    void BeginRendering(VkImageView renderTarget, VkImageView depthRenderTarget, VkRect2D renderarea);
    void EndRendering();

    inline VkCommandBuffer* GetCommandBuffer()
    {
        return &m_CommandBuffer;
    }

private:
    VkDevice m_Device;
    VkCommandBufferLevel m_Level;
    VkCommandPool m_CommandPool;
    VkCommandBuffer m_CommandBuffer;

    bool m_Valid;

    VkResult SetObjectName(PFN_vkSetDebugUtilsObjectNameEXT func, VkDevice device, VkObjectType objectType, uint64_t objectHandle, const char* name);
};

class FVkSingleTimeCommandBuffer
{
public:
    FVkSingleTimeCommandBuffer(VkDevice device, VkCommandPool pool);
    ~FVkSingleTimeCommandBuffer();

    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask,
                               uint32_t mipMapCount, uint32_t layerCount);
    void CopyBufferToImage(VkBuffer buffer, VkImage image, VkExtent2D imageExtent, uint32_t layerSize, uint32_t layerCount);
    void Submit(VkQueue queue);

    void GenerateMipMaps(VkPhysicalDevice physicalDevice, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    inline VkCommandBuffer GetCommandBuffer()
    {
        return m_CommandBuffer;
    }

private:
    VkDevice m_Device;
    VkCommandPool m_CommandPool;
    VkCommandBuffer m_CommandBuffer;
    VkFence m_Fence;
};
