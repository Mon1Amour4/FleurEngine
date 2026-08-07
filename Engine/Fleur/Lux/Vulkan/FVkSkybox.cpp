#include "FVkSkybox.h"

#include <Graphics.hpp>
#include <array>


const Fleur::Vec3 FVkSkybox::m_Vertices[m_VertexCount] = {
    // back
    {-1.f, 1.f, -1.f},
    {-1.f, -1.f, -1.f},
    {1.f, -1.f, -1.f},
    {1.f, -1.f, -1.f},
    {1.f, 1.f, -1.f},
    {-1.f, 1.f, -1.f},

    // left
    {-1.f, -1.f, 1.f},
    {-1.f, -1.f, -1.f},
    {-1.f, 1.f, -1.f},
    {-1.f, 1.f, -1.f},
    {-1.f, 1.f, 1.f},
    {-1.f, -1.f, 1.f},

    // right
    {1.f, -1.f, -1.f},
    {1.f, -1.f, 1.f},
    {1.f, 1.f, 1.f},
    {1.f, 1.f, 1.f},
    {1.f, 1.f, -1.f},
    {1.f, -1.f, -1.f},

    // front
    {-1.f, -1.f, 1.f},
    {-1.f, 1.f, 1.f},
    {1.f, 1.f, 1.f},
    {1.f, 1.f, 1.f},
    {1.f, -1.f, 1.f},
    {-1.f, -1.f, 1.f},

    // top
    {-1.f, 1.f, -1.f},
    {1.f, 1.f, -1.f},
    {1.f, 1.f, 1.f},
    {1.f, 1.f, 1.f},
    {-1.f, 1.f, 1.f},
    {-1.f, 1.f, -1.f},

    // bottom
    {-1.f, -1.f, -1.f},
    {-1.f, -1.f, 1.f},
    {1.f, -1.f, -1.f},
    {1.f, -1.f, -1.f},
    {-1.f, -1.f, 1.f},
    {1.f, -1.f, 1.f}};


FVkSkybox::FVkSkybox()
    : m_Device(nullptr)
    , m_PhysicalDevice(nullptr)
    , m_Pipeline(nullptr)
    , m_Sampler(nullptr)
    , m_SkyboxDescriptorPool(nullptr)
    , m_SkyboxDescriptorSet(nullptr)
    , m_SkyboxShader(nullptr)
    , m_VertexBuffer(nullptr)
    , m_ColorFormat(VK_FORMAT_UNDEFINED)
    , m_DepthFormat(VK_FORMAT_UNDEFINED)
    , m_Extent(0, 0)
    , m_DefaultViewport(0, 0)
    , m_DefaultRect({0, 0}, {0, 0})
    , m_UniformBuffer(nullptr)
    , m_SizeOfUniformBuffer(sizeof(Fleur::Mat4) * 2)
{
}

FVkSkybox::~FVkSkybox()
{

    if (m_SkyboxDescriptorPool)
        vkDestroyDescriptorPool(m_Device, m_SkyboxDescriptorPool, nullptr);

    if (m_Sampler)
        vkDestroySampler(m_Device, m_Sampler, nullptr);
}

void FVkSkybox::Create(const FVkDevice* device, const FVkSwapchain* swapchain, VkImageView imageView, vk::FVkShader* skyboxShader,
                       VkSampleCountFlagBits sampleCount, VkFormat depthFormat)
{
    m_Device = device->GetLogicalDevice();
    m_PhysicalDevice = device->GetPhysicalDevice();
    m_SkyboxShader = skyboxShader;
    m_ColorFormat = swapchain->GetImageFormat();
    m_Extent = swapchain->GetSwapchainExtent();
    m_DefaultViewport = {.x = 0, .y = 0, .width = (float)m_Extent.width, .height = (float)m_Extent.height, .minDepth = 0, .maxDepth = 1.0f};
    m_DefaultRect = {
        .offset = VkOffset2D{.x = 0, .y = 0},
        .extent = m_Extent,
    };
    m_SampleCount = sampleCount;
    m_DepthFormat = depthFormat;

    // 1. Pipeline

    vk::GetPipelineInfo pipelineInfo{};
    pipelineInfo.cullMode = VK_CULL_MODE_NONE;
    pipelineInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    pipelineInfo.depthTestEnable = false;
    pipelineInfo.depthWriteEnable = false;
    pipelineInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    pipelineInfo.samplesCount = m_SampleCount;
    pipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInfo.colorFormat = m_ColorFormat;
    pipelineInfo.depthFormat = m_DepthFormat;

    m_PipelineLayout = std::make_shared<FVkPipelineLayout>();
    m_PipelineLayout->Init(m_Device, *m_SkyboxShader);
    m_Pipeline = &m_PipelineCache.Get(*m_SkyboxShader, pipelineInfo, m_PipelineLayout);
    // 3. Descriptor pool
    createSkyboxDescriptorPool();
    // 4. Sampler
    createSkyboxSampler();
    // 5. Uniform Buffer
    m_UniformBuffer = std::make_unique<FVkBuffer>();
    m_UniformBuffer->Init(m_Device, m_PhysicalDevice, device->GetMemoryTracker(), FVkAllocationCategory::Buffer,
                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, m_SizeOfUniformBuffer, m_SizeOfUniformBuffer);
    std::array<Fleur::Mat4, 2> initialMatrices = {Fleur::Mat4(1.0f), Fleur::Mat4(1.0f)};
    m_UniformBuffer->MemCopy(initialMatrices.data(), m_SizeOfUniformBuffer);
    // 6. Descriptor Set
    createSkyboxDescriptorSet(imageView);
    // 7. VertexBuffer
    m_VertexBuffer = std::make_unique<FVkBuffer>();
    m_VertexBuffer->Init(m_Device, m_PhysicalDevice, device->GetMemoryTracker(), FVkAllocationCategory::Buffer,
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, m_VertexBufferSize, sizeof(Fleur::Vec3));
    // 8. Copy vertices to vertex buffer
    m_VertexBuffer->MemCopy(m_Vertices, m_VertexBufferSize);
}

void FVkSkybox::createSkyboxDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> descriptorPoolSize{};
    descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorPoolSize[0].descriptorCount = 1;
    descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSize[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets = 1,
        .poolSizeCount = descriptorPoolSize.size(),
        .pPoolSizes = descriptorPoolSize.data(),
    };

    VK_CHECK(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_SkyboxDescriptorPool));
}
void FVkSkybox::createSkyboxSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    VkSampler sampler{};
    VK_CHECK(vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_Sampler));
}
void FVkSkybox::createSkyboxDescriptorSet(VkImageView imageView)
{
    const VkDescriptorSetLayout skyboxDescriptorSetLayout = m_PipelineLayout->GetSetLayout(0);
    assert(skyboxDescriptorSetLayout != VK_NULL_HANDLE);
    VkDescriptorSetAllocateInfo skyboxDescriptorSetAllocInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                                             .descriptorPool = m_SkyboxDescriptorPool,
                                                             .descriptorSetCount = 1,
                                                             .pSetLayouts = &skyboxDescriptorSetLayout};

    VK_CHECK(vkAllocateDescriptorSets(m_Device, &skyboxDescriptorSetAllocInfo, &m_SkyboxDescriptorSet));

    VkDescriptorBufferInfo uniformBufferInfo{.buffer = m_UniformBuffer->GetBuffer(), .offset = VkDeviceSize{0}, .range = m_SizeOfUniformBuffer};
    VkWriteDescriptorSet descriptorUniformBufferWrite{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_SkyboxDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &uniformBufferInfo,
    };
    vkUpdateDescriptorSets(m_Device, 1, &descriptorUniformBufferWrite, 0, nullptr);

    VkDescriptorImageInfo imageSamplerInfo{};
    imageSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageSamplerInfo.imageView = imageView;
    imageSamplerInfo.sampler = m_Sampler;

    VkWriteDescriptorSet descriptorImageWrites{};
    descriptorImageWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorImageWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorImageWrites.dstSet = m_SkyboxDescriptorSet;
    descriptorImageWrites.dstBinding = 1;
    descriptorImageWrites.dstArrayElement = 0;
    descriptorImageWrites.descriptorCount = 1;
    descriptorImageWrites.pImageInfo = &imageSamplerInfo;

    vkUpdateDescriptorSets(m_Device, 1, &descriptorImageWrites, 0, nullptr);
}

void FVkSkybox::updateSkyboxDescriptorSet(VkImageView imageView)
{
    VkDescriptorImageInfo imageSamplerInfo{};
    imageSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageSamplerInfo.imageView = imageView;
    imageSamplerInfo.sampler = m_Sampler;

    VkWriteDescriptorSet descriptorImageWrites{};
    descriptorImageWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorImageWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorImageWrites.dstSet = m_SkyboxDescriptorSet;
    descriptorImageWrites.dstBinding = 1;
    descriptorImageWrites.dstArrayElement = 0;
    descriptorImageWrites.descriptorCount = 1;
    descriptorImageWrites.pImageInfo = &imageSamplerInfo;

    vkUpdateDescriptorSets(m_Device, 1, &descriptorImageWrites, 0, nullptr);
}

void FVkSkybox::SetSkybox(VkImageView imageView)
{
    updateSkyboxDescriptorSet(imageView);
}

void FVkSkybox::Record(VkCommandBuffer& cmd, VkExtent2D swapchainExtent, const Fleur::Graphics::SFLCameraData& cameraData)
{
    VkDeviceSize offsets{0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_VertexBuffer->GetBuffer(), &offsets);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipeline());

    vkCmdSetViewport(cmd, 0, 1, &m_DefaultViewport);
    vkCmdSetScissor(cmd, 0, 1, &m_DefaultRect);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipelineLayout(), 0, 1, &m_SkyboxDescriptorSet, 0, nullptr);

    m_UniformBuffer->MemCopy(&cameraData.view, sizeof(Fleur::Mat4) * 2);

    vkCmdDraw(cmd, m_VertexCount, 1, 0, 0);
}
