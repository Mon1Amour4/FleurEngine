#pragma once

#include <vulkan/vulkan.h>

#include <span>
#include <vector>

namespace vk::abstraction
{

class DescriptorAllocator
{
public:
    struct PoolSizeRatio
    {
        VkDescriptorType type;
        float ratio;
    };

    void init(VkDevice device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
    void clear_pools(VkDevice device);
    void destroy_pools(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext = nullptr);

private:
    VkDescriptorPool get_pool(VkDevice device);
    VkDescriptorPool create_pool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

    uint32_t m_SetsPerPool;
    std::vector<PoolSizeRatio> m_Ratios;
    std::vector<VkDescriptorPool> m_FullPulls;
    std::vector<VkDescriptorPool> m_ReadyPools;
};

}  // namespace vk::abstraction
