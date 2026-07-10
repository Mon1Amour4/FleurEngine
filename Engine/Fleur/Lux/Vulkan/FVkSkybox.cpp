#include "FVkSkybox.h"

#include <Graphics.hpp>
#include <array>


const glm::vec3 FVkSkybox::m_Vertices[m_VertexCount] = {
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
    , m_DescriptorSetLayout(nullptr)
    , m_Pipeline(nullptr)
    , m_Sampler(nullptr)
    , m_DescriptorPool(nullptr)
    , m_DescriptorSet(nullptr)
    , m_SkyboxShader(nullptr)
    , m_VertexBuffer(nullptr)
    , m_ColorFormat(VK_FORMAT_UNDEFINED)
    , m_DepthFormat(VK_FORMAT_UNDEFINED)
    , m_Extent(0, 0)
    , m_DefaultViewport(0, 0)
    , m_DefaultRect({0, 0}, {0, 0})
    , m_UniformBuffer(nullptr)
    , m_SizeOfUniformBuffer(sizeof(glm::mat4) * 2)
{
}

FVkSkybox::~FVkSkybox()
{
    delete m_Pipeline;
    delete m_DescriptorSetLayout;
    delete m_VertexBuffer;
    delete m_UniformBuffer;

    if (m_DescriptorPool)
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

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

    // 1. Descriptor set layout
    createDescriptorSetLayout();
    // 2. Pipeline

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

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {m_DescriptorSetLayout->GetDescriptorSetLayout()};
    m_Pipeline = m_SkyboxShader->GetPipeline(pipelineInfo, descriptorSetLayouts);
    // 4. Descriptor pool
    createDescriptorPool();
    // 5. Sampler
    createSampler();
    // 6. Uniform Buffer
    m_UniformBuffer = new FVkBuffer();
    m_UniformBuffer->Init(m_Device, m_PhysicalDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, m_SizeOfUniformBuffer, m_SizeOfUniformBuffer);
    std::array<glm::mat4, 2> initialMatrices = {glm::mat4(1.0f), glm::mat4(1.0f)};
    m_UniformBuffer->MemCopy(initialMatrices.data(), m_SizeOfUniformBuffer);
    // 7. Descriptor Set
    createDescriptorSet(imageView);
    // 8. VertexBuffer
    m_VertexBuffer = new FVkBuffer();
    m_VertexBuffer->Init(m_Device, m_PhysicalDevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, m_VertexBufferSize,
                         sizeof(glm::vec3));
    // 9. Copy vertices to vertex buffer
    m_VertexBuffer->MemCopy(m_Vertices, m_VertexBufferSize);
}

void FVkSkybox::createDescriptorSetLayout()
{
    m_DescriptorSetLayout = (FVkDescriptorSetLayout::Builder(m_Device)
                                 .add(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1)
                                 .add(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
                                 .build(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT));
}

void FVkSkybox::createDescriptorPool()
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

    if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
        assert(false);
}
void FVkSkybox::createSampler()
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
    if (vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS)
    {
        assert(true);
    }
}
void FVkSkybox::createDescriptorSet(VkImageView imageView)
{
    auto skyboxDescriptorSetLayout = m_DescriptorSetLayout->GetDescriptorSetLayout();
    VkDescriptorSetAllocateInfo skyboxDescriptorSetAllocInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                                             .descriptorPool = m_DescriptorPool,
                                                             .descriptorSetCount = 1,
                                                             .pSetLayouts = &skyboxDescriptorSetLayout};

    if (vkAllocateDescriptorSets(m_Device, &skyboxDescriptorSetAllocInfo, &m_DescriptorSet) != VK_SUCCESS)
        assert(true);

    VkDescriptorBufferInfo uniformBufferInfo{.buffer = m_UniformBuffer->GetBuffer(), .offset = VkDeviceSize{0}, .range = m_SizeOfUniformBuffer};
    VkWriteDescriptorSet descriptorUniformBufferWrite{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_DescriptorSet,
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
    descriptorImageWrites.dstSet = m_DescriptorSet;
    descriptorImageWrites.dstBinding = 1;
    descriptorImageWrites.dstArrayElement = 0;
    descriptorImageWrites.descriptorCount = 1;
    descriptorImageWrites.pImageInfo = &imageSamplerInfo;

    vkUpdateDescriptorSets(m_Device, 1, &descriptorImageWrites, 0, nullptr);
}

void FVkSkybox::updateDescriptorSet(VkImageView imageView)
{
    VkDescriptorImageInfo imageSamplerInfo{};
    imageSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageSamplerInfo.imageView = imageView;
    imageSamplerInfo.sampler = m_Sampler;

    VkWriteDescriptorSet descriptorImageWrites{};
    descriptorImageWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorImageWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorImageWrites.dstSet = m_DescriptorSet;
    descriptorImageWrites.dstBinding = 1;
    descriptorImageWrites.dstArrayElement = 0;
    descriptorImageWrites.descriptorCount = 1;
    descriptorImageWrites.pImageInfo = &imageSamplerInfo;

    vkUpdateDescriptorSets(m_Device, 1, &descriptorImageWrites, 0, nullptr);
}

void FVkSkybox::SetSkybox(VkImageView imageView)
{
    updateDescriptorSet(imageView);
}

void FVkSkybox::Record(VkCommandBuffer& cmd, VkExtent2D swapchainExtent, const Fleur::Graphics::SFLCameraData& cameraData)
{
    VkDeviceSize offsets{0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_VertexBuffer->GetBuffer(), &offsets);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipeline());

    vkCmdSetViewport(cmd, 0, 1, &m_DefaultViewport);
    vkCmdSetScissor(cmd, 0, 1, &m_DefaultRect);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipelineLayout(), 0, 1, &m_DescriptorSet, 0, nullptr);

    m_UniformBuffer->MemCopy(&cameraData.view, sizeof(glm::mat4) * 2);

    vkCmdDraw(cmd, m_VertexCount, 1, 0, 0);
}
