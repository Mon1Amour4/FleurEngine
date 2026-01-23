#pragma once

#include <vulkan/vulkan.h>

#include <vector>

struct SFLPushConstant
{
    uint32_t albedoIdx;
};

class FVkCommandPool
{
public:
    FVkCommandPool();
    ~FVkCommandPool();
    void Init(VkDevice device, VkCommandPoolCreateFlagBits usage, uint32_t queueFamilyIndex);

    inline VkCommandPool Pool() const
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
    void Begin(VkRenderPass renderPass);
    void End();

    void BindPipeline(VkPipeline pipeline);
    void SetViewport(VkViewport viewport);
    void SetScissors(VkRect2D scissors);
    void BindVertexBuffer(VkBuffer* buffer);
    void BindIndexBuffer(VkBuffer* buffer, VkIndexType indextype);
    void BindDescriptorSet(VkPipelineLayout pipelineLayout, VkDescriptorSet* descriptorSet);
    void PushConstant(VkPipelineLayout pipelineLayout, VkShaderStageFlagBits shaderStage, SFLPushConstant constant);
    void DrawIndexed(uint32_t indexCount, size_t indexOffset, size_t vertexOffset);
    void Bi

    private:
    VkDevice m_Device;
    VkCommandBufferLevel m_Level;
    VkCommandPool m_CommandPool;
    VkCommandBuffer m_CommandBuffer;

    bool m_Valid;

};