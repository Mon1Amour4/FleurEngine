#include "FVkPipeline.h"

#include <cassert>

FVkPipeline::FVkPipeline()
    : m_Device(nullptr)
    , m_Pipeline(nullptr)
    , m_PipelineLayout(nullptr)
{
}

FVkPipeline::~FVkPipeline()
{
    if (m_Pipeline)
        vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
    if (m_PipelineLayout)
        vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
}

void FVkPipeline::Init(VkDevice device, FGraphicsPipelineDesc& desc)
{
    assert(desc.cullMode != VK_CULL_MODE_FLAG_BITS_MAX_ENUM);
    assert(desc.frontFace != VK_FRONT_FACE_MAX_ENUM);
    assert(desc.shaderStages && desc.shaderStages->size() > 0);

    m_Device = device;
    m_DescriptorSetLayouts = desc.descriptorSetLayouts;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                                  .setLayoutCount = (uint32_t)m_DescriptorSetLayouts.size(),
                                                  .pSetLayouts = m_DescriptorSetLayouts.data(),
                                                  .pushConstantRangeCount = desc.pushConstants ? (uint32_t)desc.pushConstants->size() : 0,
                                                  .pPushConstantRanges = desc.pushConstants ? desc.pushConstants->data() : nullptr};


    VK_CHECK(vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout));

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = desc.topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkRect2D scissor{.offset = VkOffset2D{0, 0}, .extent = VkExtent2D{1, 1}};

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = desc.samplesCount;
    multisampling.minSampleShading = 1.0f;           // Optional
    multisampling.pSampleMask = nullptr;             // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
    multisampling.alphaToOneEnable = VK_FALSE;       // Optional

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = desc.cullMode;
    rasterizer.frontFace = desc.frontFace;
    rasterizer.depthBiasEnable = desc.depthBiasEnable ? VK_TRUE : VK_FALSE;
    rasterizer.depthBiasConstantFactor = desc.depthBiasConstantFactor;
    rasterizer.depthBiasClamp = desc.depthBiasClamp;
    rasterizer.depthBiasSlopeFactor = desc.depthBiasSlopeFactor;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = desc.colorBlendAttachment.blendEnable;
    colorBlendAttachment.colorWriteMask = desc.colorBlendAttachment.colorWriteMask;
    colorBlendAttachment.srcColorBlendFactor = desc.colorBlendAttachment.srcColorBlendFactor;
    colorBlendAttachment.dstColorBlendFactor = desc.colorBlendAttachment.dstColorBlendFactor;
    colorBlendAttachment.colorBlendOp = desc.colorBlendAttachment.colorBlendOp;
    colorBlendAttachment.srcAlphaBlendFactor = desc.colorBlendAttachment.srcAlphaBlendFactor;
    colorBlendAttachment.dstAlphaBlendFactor = desc.colorBlendAttachment.dstAlphaBlendFactor;
    colorBlendAttachment.alphaBlendOp = desc.colorBlendAttachment.alphaBlendOp;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;  // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;  // Optional
    colorBlending.blendConstants[1] = 0.0f;  // Optional
    colorBlending.blendConstants[2] = 0.0f;  // Optional
    colorBlending.blendConstants[3] = 0.0f;  // Optional

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineDepthStencilStateCreateInfo depthStencil{.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                                                       .depthTestEnable = desc.depthTestEnable,
                                                       .depthWriteEnable = desc.depthWriteEnable,
                                                       .depthCompareOp = desc.depthCompareOp,
                                                       .depthBoundsTestEnable = VK_FALSE,
                                                       .stencilTestEnable = VK_FALSE,
                                                       .front = {},
                                                       .back = {},
                                                       .minDepthBounds = 0.0f,
                                                       .maxDepthBounds = 1.0f};

    VkPipelineRenderingCreateInfo renderingInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
                                                .pNext = nullptr,
                                                .colorAttachmentCount = desc.colorAttachmentCount,
        .pColorAttachmentFormats = renderingInfo.pColorAttachmentFormats = desc.colorAttachmentCount > 0 ? &desc.colorFormat : nullptr,
                                                .depthAttachmentFormat = desc.depthFormat,
                                                .stencilAttachmentFormat = VK_FORMAT_UNDEFINED};

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = desc.shaderStages->size();
    pipelineInfo.pStages = desc.shaderStages->data();
    pipelineInfo.pVertexInputState = desc.pVertexInputState;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;  // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = nullptr;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
    pipelineInfo.basePipelineIndex = -1;               // Optional

    VK_CHECK(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline));
}

uint32_t FVkPipeline::GetBindingIdx()
{
    return 0;
}
