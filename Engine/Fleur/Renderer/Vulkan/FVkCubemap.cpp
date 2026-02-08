#include "FVkCubemap.h"

FVkSkybox::FVkSkybox()
    : m_Device(nullptr)
    , m_VertexShader(nullptr)
    , m_FragmentShader(nullptr)
    , m_Texture(nullptr)
    , m_Pipeline(nullptr)
    , m_VertexInput(nullptr)
    , m_RenderPass(nullptr)
    , m_PhysicalDevice(nullptr)
{
}

FVkSkybox::~FVkSkybox()
{
    delete m_Texture;
    delete m_VertexInput;
    delete m_Pipeline;

    vkDestroyShaderModule(m_Device, m_VertexShader, nullptr);
    vkDestroyShaderModule(m_Device, m_FragmentShader, nullptr);
}

void FVkSkybox::Create(const FVkDevice* device, const FVkSwapchain* swapchain, VkShaderModule vertexShader, VkShaderModule fragmentShader)
{
    m_Device = device->GetLogicalDevice();
    m_PhysicalDevice = device->GetPhysicalDevice();
    m_VertexShader = vertexShader;
    m_FragmentShader = fragmentShader;

    m_VertexInput = new SFLVertexInput();
    m_VertexInput->RegisterAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);

    m_Pipeline = new FVkPipeline();

    VkViewport viewport{
        .x = 0,
        .y = 0,
        .width = extent.width,
        .height = extent.height,
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };

    CreateRenderPass(swapchain);

    SFLVertexInput vertexInput{};
    SFPipelineCreationInfo pipelineInfo{
        .device = m_Device,
        .renderPass = m_RenderPass,
        .descriptorSetLayout =,
        .vertexShader = m_VertexShader,
        .fragmentShader = m_FragmentShader,
        .pushConstantSize = 0,
        .vertexInput = m_VertexInput,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .viewport = &viewport,
        .extent = extent,
        .samplesCount = VK_SAMPLE_COUNT_1_BIT,
    };
    m_Pipeline->Init(&pipelineInfo);
}

void FVkSkybox::CreateRenderPass(const FVkSwapchain* swapchain)
{
    // VkAttachmentDescription depthAttachment{};
    // depthAttachment.format = FindDepthFormat(m_PhysicalDevice);
    // depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    // depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // VkAttachmentReference depthAttachmentRef{};
    // depthAttachmentRef.attachment = 1;
    // depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchain->GetImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = swapchain->GetImageFormat();
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;


    if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
    {
        assert(false);
    }
}
