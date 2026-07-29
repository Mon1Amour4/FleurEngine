#include "DescriptorPoolAllocator.h"

#include <cassert>

#include "VkHelper.h"

VkDescriptorPool vk::abstraction::DescriptorAllocator::get_pool(VkDevice device)
{
    VkDescriptorPool newPool;
    if (m_ReadyPools.size() != 0)
    {
        newPool = m_ReadyPools.back();
        m_ReadyPools.pop_back();
    }
    else
    {
        // need to create a new pool
        newPool = create_pool(device, m_SetsPerPool, m_Ratios);

        m_SetsPerPool *= 1.5;
        if (m_SetsPerPool > 4092)
        {
            m_SetsPerPool = 4092;
        }
    }

    return newPool;
}
VkDescriptorPool vk::abstraction::DescriptorAllocator::create_pool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios)
    {
        poolSizes.push_back(VkDescriptorPoolSize{.type = ratio.type, .descriptorCount = uint32_t(ratio.ratio * setCount)});
    }

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = setCount;
    pool_info.poolSizeCount = (uint32_t)poolSizes.size();
    pool_info.pPoolSizes = poolSizes.data();

    VkDescriptorPool newPool;
    VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &newPool));

    return newPool;
}

void vk::abstraction::DescriptorAllocator::init(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
{
    assert(maxSets > 0);

    m_Ratios.clear();

    for (auto r : poolRatios)
    {
        m_Ratios.push_back(r);
    }

    VkDescriptorPool newPool = create_pool(device, maxSets, poolRatios);

    m_SetsPerPool = maxSets * 1.5;  // grow it next allocation

    m_ReadyPools.push_back(newPool);
}

void vk::abstraction::DescriptorAllocator::clear_pools(VkDevice device)
{
    for (auto p : m_ReadyPools)
    {
        vkResetDescriptorPool(device, p, 0);
    }
    for (auto p : m_FullPulls)
    {
        vkResetDescriptorPool(device, p, 0);
        m_ReadyPools.push_back(p);
    }
    m_FullPulls.clear();
}

void vk::abstraction::DescriptorAllocator::destroy_pools(VkDevice device)
{
    for (auto p : m_ReadyPools)
    {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    m_ReadyPools.clear();
    for (auto p : m_FullPulls)
    {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    m_FullPulls.clear();
}

VkDescriptorSet vk::abstraction::DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout, uint32_t setsCount, void* pNext)
{
    // get or create a pool to allocate from
    VkDescriptorPool poolToUse = get_pool(device);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.pNext = pNext;
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = poolToUse;
    allocInfo.descriptorSetCount = setsCount;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet ds;
    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);

    // allocation failed. Try again
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
        m_FullPulls.push_back(poolToUse);

        poolToUse = get_pool(device);
        allocInfo.descriptorPool = poolToUse;

        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));
    }

    m_ReadyPools.push_back(poolToUse);
    return ds;
}

// Writes a single buffer descriptor for `binding`.
// Valid `type` values:
// - VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
// - VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
// - VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
// - VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
//
// `binding` must exist in the target descriptor set layout and be compatible with `type`.
// `buffer` must be a valid VkBuffer.
// `offset` and `size` specify the buffer range exposed through the descriptor.
void vk::abstraction::DescriptorWriter::write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
    VkDescriptorBufferInfo& info = bufferInfos.emplace_back(VkDescriptorBufferInfo{.buffer = buffer, .offset = offset, .range = size});

    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;  // left empty for now until we need to write it
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;

    writes.push_back(write);
}

void vk::abstraction::DescriptorWriter::write_image(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
    VkDescriptorImageInfo& info = imageInfos.emplace_back(VkDescriptorImageInfo{.sampler = sampler, .imageView = image, .imageLayout = layout});

    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;  // left empty for now until we need to write it
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;

    writes.push_back(write);
}

void vk::abstraction::DescriptorWriter::write_image_array(int binding, const VkDescriptorImageInfo* pImageInfos, uint32_t imageCount,
                                                           VkDescriptorType type)
{
    assert(pImageInfos != nullptr);
    assert(imageCount > 0);

    auto& infos = imageArrayInfos.emplace_back(pImageInfos, pImageInfos + imageCount);

    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;
    write.descriptorCount = imageCount;
    write.descriptorType = type;
    write.pImageInfo = infos.data();

    writes.push_back(write);
}

void vk::abstraction::DescriptorWriter::clear()
{
    imageInfos.clear();
    imageArrayInfos.clear();
    writes.clear();
    bufferInfos.clear();
}

void vk::abstraction::DescriptorWriter::update_set(VkDevice device, VkDescriptorSet set)
{
    for (VkWriteDescriptorSet& write : writes)
    {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
}
