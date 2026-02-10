#include "FVkCubemap.h"

#include <Graphics.hpp>
#include <array>

FVkSkybox::FVkSkybox()
    : m_Device(nullptr)
    , m_VertexShader(nullptr)
    , m_FragmentShader(nullptr)
    , m_Pipeline(nullptr)
    , m_VertexInput(nullptr)
    , m_RenderPass(nullptr)
    , m_PhysicalDevice(nullptr)
    , m_DescriptorSetLayout(nullptr)
    , m_DepthStencilCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
    , m_DescriptorPool(nullptr)
{
}

FVkSkybox::~FVkSkybox()
{
    delete m_VertexInput;
    delete m_Pipeline;

    if (m_VertexShader)
        vkDestroyShaderModule(m_Device, m_VertexShader, nullptr);
    if (m_FragmentShader)
        vkDestroyShaderModule(m_Device, m_FragmentShader, nullptr);

    if (m_DescriptorSetLayout)
        vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);

    if (m_DescriptorPool)
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

    if (m_Sampler)
        vkDestroySampler(m_Device, m_Sampler, nullptr);
}

void FVkSkybox::Create(const FVkDevice* device, const FVkSwapchain* swapchain, VkImageView imageView, VkShaderModule vertexShader,
                       VkShaderModule fragmentShader)
{
    m_Device = device->GetLogicalDevice();
    m_PhysicalDevice = device->GetPhysicalDevice();
    m_VertexShader = vertexShader;
    m_FragmentShader = fragmentShader;
    m_ImageView = imageView;

    m_VertexInput = new SFLVertexInput();
    m_VertexInput->RegisterAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);

    m_Pipeline = new FVkPipeline();

    VkExtent2D swapchainExtent = swapchain->GetSwapchainExtent();
    VkViewport viewport{
        .x = 0,
        .y = 0,
        .width = (float)swapchainExtent.width,
        .height = (float)swapchainExtent.height,
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };

    CreateRenderPass(swapchain);
    CreateDescriptorSetLayout();
    SFPipelineCreationInfo pipelineInfo{.device = m_Device,
                                        .renderPass = m_RenderPass,
                                        .descriptorSetLayout = m_DescriptorSetLayout,
                                        .vertexShader = m_VertexShader,
                                        .fragmentShader = m_FragmentShader,
                                        .pushConstantSize = sizeof(uint32_t),
                                        .vertexInput = m_VertexInput,
                                        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                        .viewport = &viewport,
                                        .extent = swapchainExtent,
                                        .samplesCount = VK_SAMPLE_COUNT_1_BIT,
                                        .depthStencilOp = m_DepthStencilCompareOp,
                                        .depthWriteEnable = false};
    m_Pipeline->Init(&pipelineInfo);

    CreateDescriptorPool(swapchain->GetSwapchainFramebuffersCount());
    CreateSampler();
    CreateDescriptorSets(swapchain->GetSwapchainFramebuffersCount());
}

void FVkSkybox::SetSkybox(VkImageView imageView)
{
    m_ImageView = imageView;

    // Update VkWriteDescriptorSet for VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
}

void FVkSkybox::Record(VkCommandBuffer cmd, uint32_t frame)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipeline());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipelineLayout(), 0, 1, &m_DescriptorSets[frame], 0, nullptr);

    uint32_t vertexCount = 36;

    vkCmdDraw(cmd, vertexCount, 1, 0, 0);
}

void FVkSkybox::Update(uint32_t frame, const glm::mat4& transformation)
{
    m_UniformBuffers[frame].MemCopy(&transformation, sizeof(glm::mat4));
}

void FVkSkybox::CreateRenderPass(const FVkSwapchain* swapchain)
{
    VkAttachmentDescription colorAttachment{.format = swapchain->GetImageFormat(),
                                            .samples = VK_SAMPLE_COUNT_1_BIT,
                                            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

    VkAttachmentDescription depthAttachment{.format = FindDepthFormat(m_PhysicalDevice),
                                            .samples = VK_SAMPLE_COUNT_1_BIT,
                                            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                            .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkAttachmentReference colorRef{.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkAttachmentReference depthRef{.attachment = 1, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS, .colorAttachmentCount = 1, .pColorAttachments = &colorRef, .pDepthStencilAttachment = &depthRef};

    VkSubpassDependency dependency{.srcSubpass = VK_SUBPASS_EXTERNAL,
                                   .dstSubpass = 0,
                                   .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                   .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                   .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                   .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT};

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                          .attachmentCount = attachments.size(),
                                          .pAttachments = attachments.data(),
                                          .subpassCount = 1,
                                          .pSubpasses = &subpass,
                                          .dependencyCount = 1,
                                          .pDependencies = &dependency};

    if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
    {
        assert(false);
    }
}

void FVkSkybox::CreateDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;           // if more than 1 then it means an array
    uboLayoutBinding.pImmutableSamplers = nullptr;  // Optional

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.binding = 1;
    // Max num of descriptors in this current descriptor set
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
    {
        assert(false);
    }
}

void FVkSkybox::CreateDescriptorPool(uint32_t framebufferCount)
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = framebufferCount;

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = framebufferCount;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = framebufferCount;

    if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
    {
        assert(false);
    }
}

void FVkSkybox::CreateDescriptorSets(uint32_t framebufferCount)
{
    std::vector<VkDescriptorSetLayout> layouts(framebufferCount, m_DescriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = layouts.size();
    allocInfo.pSetLayouts = layouts.data();

    m_DescriptorSets.resize(framebufferCount);
    if (vkAllocateDescriptorSets(m_Device, &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS)
    {
        assert(true);
    }

    m_UniformBuffers.resize(m_DescriptorSets.size());
    for (size_t i = 0; i < m_DescriptorSets.size(); i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_UniformBuffers[i].GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(Fleur::Graphics::SFLGeometryUBO);

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].dstSet = m_DescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_Device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

        VkDescriptorImageInfo imageSamplerInfo{};
        imageSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageSamplerInfo.imageView = m_ImageView;
        imageSamplerInfo.sampler = m_Sampler;

        std::array<VkWriteDescriptorSet, 1> descriptorImageWrites{};
        for (size_t j = 0; j < descriptorImageWrites.size(); j++)
        {
            descriptorImageWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorImageWrites[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorImageWrites[j].dstSet = m_DescriptorSets[i];
            descriptorImageWrites[j].dstBinding = 1;
            descriptorImageWrites[j].dstArrayElement = j;
            descriptorImageWrites[j].descriptorCount = 1;
            descriptorImageWrites[j].pImageInfo = &imageSamplerInfo;
        }
        vkUpdateDescriptorSets(m_Device, descriptorImageWrites.size(), descriptorImageWrites.data(), 0, nullptr);
    }
}

void FVkSkybox::CreateSampler()
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
