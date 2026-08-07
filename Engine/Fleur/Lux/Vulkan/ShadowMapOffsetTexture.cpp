#include "ShadowMapOffsetTexture.h"

#include <cassert>
#include <cmath>
#include <random>

#include "FVkBuffer.h"
#include "FVkCommand.h"
#include "VkHelper.h"

namespace
{
constexpr float kPi = 3.14159265358979323846f;
constexpr VkExtent2D kPatternExtent{128, 128};
}

ShadowMapOffsetTexture::~ShadowMapOffsetTexture()
{
    Destroy();
}

float ShadowMapOffsetTexture::jitter()
{
    static std::default_random_engine generator;
    static std::uniform_real_distribution<float> distrib(-0.5f, 0.5f);
    return distrib(generator);
}

void ShadowMapOffsetTexture::Generate(VkExtent2D extent, uint32_t filterSize)
{
    assert(filterSize > 0);

    m_Extent = extent;
    m_TextureExtent = {extent.width, extent.height, filterSize * filterSize};
    m_FilterSize = filterSize;

    const size_t texelCount = static_cast<size_t>(m_TextureExtent.width) * m_TextureExtent.height * m_TextureExtent.depth;
    m_Data.resize(texelCount * 2);

    for (uint32_t texY = 0; texY < extent.height; ++texY)
    {
        for (uint32_t texX = 0; texX < extent.width; ++texX)
        {
            for (uint32_t v = 0; v < filterSize; ++v)
            {
                for (uint32_t u = 0; u < filterSize; ++u)
                {
                    const float x = (static_cast<float>(u) + 0.5f + jitter()) / static_cast<float>(filterSize);
                    const float y = (static_cast<float>(v) + 0.5f + jitter()) / static_cast<float>(filterSize);
                    const uint32_t sampleIndex = v * filterSize + u;
                    const size_t index = (static_cast<size_t>(sampleIndex) * m_Extent.height * m_Extent.width
                                          + static_cast<size_t>(texY) * m_Extent.width + texX) * 2;

                    m_Data[index] = std::sqrt(y) * std::cos(2.0f * kPi * x);
                    m_Data[index + 1] = std::sqrt(y) * std::sin(2.0f * kPi * x);
                }
            }
        }
    }
}

void ShadowMapOffsetTexture::Create(VkDevice device, VkPhysicalDevice physicalDevice, FVkMemoryTracker& memoryTracker, VkCommandPool commandPool,
                                    VkQueue graphicsQueue, uint32_t filterSize)
{
    Destroy();
    Generate(kPatternExtent, filterSize);
    m_Device = device;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_3D;
    imageInfo.format = VK_FORMAT_R32G32_SFLOAT;
    imageInfo.extent = m_TextureExtent;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    m_Texture.CreateImage(device, physicalDevice, memoryTracker, FVkAllocationCategory::Texture, imageInfo,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    FVkBuffer stagingBuffer{};
    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(m_Data.size() * sizeof(float));
    stagingBuffer.Init(device, physicalDevice, memoryTracker, FVkAllocationCategory::Staging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       imageSize, sizeof(float) * 2);
    stagingBuffer.MemCopy(m_Data.data(), imageSize);

    {
        FVkSingleTimeCommandBuffer commandBuffer(device, commandPool);
        commandBuffer.TransitionImageLayout(m_Texture.GetImage(), imageInfo.format, VK_IMAGE_LAYOUT_UNDEFINED,
                                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, 1);
        commandBuffer.CopyBufferToImage(stagingBuffer.GetBuffer(), m_Texture.GetImage(), m_TextureExtent,
                                        static_cast<uint32_t>(imageSize), 1);
        commandBuffer.TransitionImageLayout(m_Texture.GetImage(), imageInfo.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, 1);
        commandBuffer.Submit(graphicsQueue);
    }

    m_Texture.CreateImageView();

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.maxLod = 0.0f;
    VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler));

}

void ShadowMapOffsetTexture::UpdateDescriptorSet(VkDescriptorSet descriptorSet) const
{
    assert(descriptorSet != VK_NULL_HANDLE);
    VkDescriptorImageInfo imageInfoDescriptor{};
    imageInfoDescriptor.sampler = m_Sampler;
    imageInfoDescriptor.imageView = m_Texture.GetImageView();
    imageInfoDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfoDescriptor;
    vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);
}

void ShadowMapOffsetTexture::Destroy()
{
    if (m_Device == VK_NULL_HANDLE)
        return;

    m_Texture.Destroy();
    if (m_Sampler != VK_NULL_HANDLE)
        vkDestroySampler(m_Device, m_Sampler, nullptr);

    m_Sampler = VK_NULL_HANDLE;
    m_Device = VK_NULL_HANDLE;
}
