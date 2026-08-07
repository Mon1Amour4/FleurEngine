#include "FVkShader.h"
#include "FVkPipelineLayout.h"

#include <algorithm>
#include <cassert>

#include <Fleur/Log.h>

vk::FVkShader::FVkShader()
    : m_Device(nullptr)
{
}

vk::FVkShader::~FVkShader()
{
    DestroyModules();
}

void vk::FVkShader::DestroyModules()
{
    if (m_Device != VK_NULL_HANDLE)
    {
        if (m_VertexShader.shaderModule)
            vkDestroyShaderModule(m_Device, m_VertexShader.shaderModule, nullptr);
        if (m_GeometryShader.shaderModule)
            vkDestroyShaderModule(m_Device, m_GeometryShader.shaderModule, nullptr);
        if (m_FragmentShader.shaderModule)
            vkDestroyShaderModule(m_Device, m_FragmentShader.shaderModule, nullptr);
    }

    m_VertexShader.shaderModule = VK_NULL_HANDLE;
    m_GeometryShader.shaderModule = VK_NULL_HANDLE;
    m_FragmentShader.shaderModule = VK_NULL_HANDLE;
}

void vk::FVkShader::Init(VkDevice device, ShaderCreateInfo& info)
{
    assert(info.pVertexData);
    assert(info.pFragmentData);

    m_Device = device;

    VkShaderModuleCreateInfo vertexcreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = info.vertexSize, .pCode = reinterpret_cast<const uint32_t*>(info.pVertexData)};

    VK_CHECK(vkCreateShaderModule(m_Device, &vertexcreateInfo, nullptr, &m_VertexShader.shaderModule));

    if (info.pGeometryData && info.geometrySize > 0)
    {
        VkShaderModuleCreateInfo geometryCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = info.geometrySize, .pCode = reinterpret_cast<const uint32_t*>(info.pGeometryData)};
        VK_CHECK(vkCreateShaderModule(m_Device, &geometryCreateInfo, nullptr, &m_GeometryShader.shaderModule));
    }

    VkShaderModuleCreateInfo fragmentcreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = info.fragmentSize, .pCode = reinterpret_cast<const uint32_t*>(info.pFragmentData)};

    VK_CHECK(vkCreateShaderModule(m_Device, &fragmentcreateInfo, nullptr, &m_FragmentShader.shaderModule));

    try
    {
        if (!getReflection(m_VertexShader, info.pVertexData, info.vertexSize))
        {
            DestroyModules();
            return;
        }
        if (m_GeometryShader.shaderModule)
        {
            if (!getReflection(m_GeometryShader, info.pGeometryData, info.geometrySize))
            {
                DestroyModules();
                return;
            }
        }
        if (!getReflection(m_FragmentShader, info.pFragmentData, info.fragmentSize))
        {
            DestroyModules();
            return;
        }
        mergePushConstants();
    }
    catch (...)
    {
        DestroyModules();
        throw;
    }

    std::sort(m_Reflection.descriptorBindings.begin(), m_Reflection.descriptorBindings.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.set != rhs.set)
            return lhs.set < rhs.set;
        return lhs.binding < rhs.binding;
    });
    m_Reflection.pushConstants = m_PushConstants;
}


bool vk::FVkShader::getReflection(ShaderData& shaderData, const void* const pVertexData, size_t vertexSize)
{
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(vertexSize, pVertexData, &module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    uint32_t var_count = 0;
    result = spvReflectEnumerateInputVariables(&module, &var_count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    SpvReflectInterfaceVariable** input_vars = (SpvReflectInterfaceVariable**)malloc(var_count * sizeof(SpvReflectInterfaceVariable*));
    const auto failReflection = [&]() {
        free(input_vars);
        spvReflectDestroyShaderModule(&module);
        return false;
    };
    result = spvReflectEnumerateInputVariables(&module, &var_count, input_vars);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    shaderData.entryPoint = module.entry_point_name;
    shaderData.shaderStage = convertReflectionShaderStage(module.shader_stage);

    VkPipelineShaderStageCreateInfo shaderStage{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                .stage = shaderData.shaderStage,
                                                .module = shaderData.shaderModule,
                                                .pName = shaderData.entryPoint.c_str()};
    m_ShaderStages.push_back(shaderStage);

    if (shaderData.shaderStage == VK_SHADER_STAGE_VERTEX_BIT)
    {
        m_VertexInput.bindingDescription = VkVertexInputBindingDescription{.binding = 0, .stride = 0, .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

        uint32_t inputOffset = 0;
        for (size_t i = 0; i < module.entry_points->input_variable_count; i++)
        {
            SpvReflectInterfaceVariable* inputVariable = module.entry_points->input_variables[i];
            uint32_t location = inputVariable->location;
            if (location == std::numeric_limits<uint32_t>::max())
                continue;

            VkFormat format = convertReflectionFormat(inputVariable->format);
            uint32_t offset = inputOffset;
            m_VertexInput.m_VertexInputAttributes.emplace_back(location, m_VertexInput.bindingDescription.binding, format, offset);

            uint32_t componentCount = inputVariable->numeric.vector.component_count;
            uint32_t componentSize = inputVariable->numeric.scalar.width / 8;
            inputOffset += componentCount * componentSize;
        }
        m_VertexInput.bindingDescription.stride = inputOffset;


        const bool hasVertexAttributes = !m_VertexInput.m_VertexInputAttributes.empty();
        m_VertexInput.createInfo =
            VkPipelineVertexInputStateCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                                                 .vertexBindingDescriptionCount = hasVertexAttributes ? 1u : 0u,
                                                 .pVertexBindingDescriptions = hasVertexAttributes ? &m_VertexInput.bindingDescription : nullptr,
                                                 .vertexAttributeDescriptionCount = static_cast<uint32_t>(m_VertexInput.m_VertexInputAttributes.size()),
                                                 .pVertexAttributeDescriptions = hasVertexAttributes ? m_VertexInput.m_VertexInputAttributes.data() : nullptr};
    }

    for (size_t i = 0; i < module.descriptor_set_count; i++)
    {
        SpvReflectDescriptorSet* currentDescriptorSet = module.descriptor_sets + i;
        if (currentDescriptorSet->binding_count != 0)
        {
            for (size_t j = 0; j < currentDescriptorSet->binding_count; j++)
            {
                SpvReflectDescriptorBinding* reflectBinding = currentDescriptorSet->bindings[j];
                const VkDescriptorType descriptorType = convertReflectionDescriptorType(reflectBinding->descriptor_type);
                const uint32_t descriptorCount = reflectBinding->count;
                if (descriptorCount == 0)
                {
                    FL_CORE_ERROR("[Vulkan] Shader reflection failed: stage={} has unsupported runtime descriptor array at set={}, binding={}",
                                  static_cast<uint32_t>(shaderData.shaderStage), currentDescriptorSet->set, reflectBinding->binding);
                    return failReflection();
                }

                auto existing = std::find_if(m_Reflection.descriptorBindings.begin(), m_Reflection.descriptorBindings.end(), [&](const auto& binding) {
                    return binding.set == currentDescriptorSet->set && binding.binding == reflectBinding->binding;
                });

                if (existing == m_Reflection.descriptorBindings.end())
                {
                    m_Reflection.descriptorBindings.push_back(FVkDescriptorBindingReflection{
                        .set = currentDescriptorSet->set,
                        .binding = reflectBinding->binding,
                        .descriptorType = descriptorType,
                        .descriptorCount = descriptorCount,
                        .stageFlags = static_cast<VkShaderStageFlags>(shaderData.shaderStage),
                    });
                }
                else
                {
                    if (existing->descriptorType != descriptorType || existing->descriptorCount != descriptorCount)
                    {
                        FL_CORE_ERROR("[Vulkan] Shader reflection failed: incompatible descriptor at set={}, binding={}, stage={}", currentDescriptorSet->set,
                                      reflectBinding->binding, static_cast<uint32_t>(shaderData.shaderStage));
                        return failReflection();
                    }
                    existing->stageFlags |= shaderData.shaderStage;
                }
            }
        }
    }

    for (size_t i = 0; i < module.push_constant_block_count; i++)
    {
        SpvReflectBlockVariable* currentReflectionPushConstant = module.push_constant_blocks + i;
        auto& pushConstant = m_PushConstants.emplace_back();
        pushConstant.stageFlags = shaderData.shaderStage;
        pushConstant.offset = currentReflectionPushConstant->offset;
        pushConstant.size = currentReflectionPushConstant->size;
    }
    free(input_vars);

    // Output variables, descriptor bindings, descriptor sets, and push constants
    // can be enumerated and extracted using a similar mechanism.

    // Destroy the reflection data when no longer required.
    spvReflectDestroyShaderModule(&module);
    return true;
}

void vk::FVkShader::mergePushConstants()
{
    std::vector<VkPushConstantRange> merged;
    merged.reserve(m_PushConstants.size());

    for (const VkPushConstantRange& range : m_PushConstants)
    {
        auto it = std::find_if(merged.begin(), merged.end(),
                               [&](const VkPushConstantRange& existing) { return existing.offset == range.offset && existing.size == range.size; });

        if (it != merged.end())
        {
            it->stageFlags |= range.stageFlags;
        }
        else
        {
            merged.push_back(range);
        }
    }

    m_PushConstants = std::move(merged);
}

// ---------- reflection to vk ----------
VkShaderStageFlagBits vk::FVkShader::convertReflectionShaderStage(SpvReflectShaderStageFlagBits stage)
{
    switch (stage)
    {
    case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
        return VK_SHADER_STAGE_VERTEX_BIT;

    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

    case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
        return VK_SHADER_STAGE_GEOMETRY_BIT;

    case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
        return VK_SHADER_STAGE_FRAGMENT_BIT;

    case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
        return VK_SHADER_STAGE_COMPUTE_BIT;

    case SPV_REFLECT_SHADER_STAGE_TASK_BIT_NV:
        return VK_SHADER_STAGE_TASK_BIT_EXT;

    case SPV_REFLECT_SHADER_STAGE_MESH_BIT_NV:
        return VK_SHADER_STAGE_MESH_BIT_EXT;

    case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR:
        return VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR:
        return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

    case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
        return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR:
        return VK_SHADER_STAGE_MISS_BIT_KHR;

    case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR:
        return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;

    case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR:
        return VK_SHADER_STAGE_CALLABLE_BIT_KHR;

    default:
        return static_cast<VkShaderStageFlagBits>(0);
    }
}
VkDescriptorType vk::FVkShader::convertReflectionDescriptorType(SpvReflectDescriptorType type)
{
    switch (type)
    {
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
        return VK_DESCRIPTOR_TYPE_SAMPLER;

    case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;

    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;

    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;

    case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

    case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    default:
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}
VkFormat vk::FVkShader::convertReflectionFormat(SpvReflectFormat format)
{
    switch (format)
    {
    case SPV_REFLECT_FORMAT_UNDEFINED:
        return VK_FORMAT_UNDEFINED;
    case SPV_REFLECT_FORMAT_R16_UINT:
        return VK_FORMAT_R16_UINT;
    case SPV_REFLECT_FORMAT_R16_SINT:
        return VK_FORMAT_R16_SINT;
    case SPV_REFLECT_FORMAT_R16_SFLOAT:
        return VK_FORMAT_R16_SFLOAT;
    case SPV_REFLECT_FORMAT_R16G16_UINT:
        return VK_FORMAT_R16G16_UINT;
    case SPV_REFLECT_FORMAT_R16G16_SINT:
        return VK_FORMAT_R16G16_SINT;
    case SPV_REFLECT_FORMAT_R16G16_SFLOAT:
        return VK_FORMAT_R16G16_SFLOAT;
    case SPV_REFLECT_FORMAT_R16G16B16_UINT:
        return VK_FORMAT_R16G16B16_UINT;
    case SPV_REFLECT_FORMAT_R16G16B16_SINT:
        return VK_FORMAT_R16G16B16_SINT;
    case SPV_REFLECT_FORMAT_R16G16B16_SFLOAT:
        return VK_FORMAT_R16G16B16_SFLOAT;
    case SPV_REFLECT_FORMAT_R16G16B16A16_UINT:
        return VK_FORMAT_R16G16B16A16_UINT;
    case SPV_REFLECT_FORMAT_R16G16B16A16_SINT:
        return VK_FORMAT_R16G16B16A16_SINT;
    case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case SPV_REFLECT_FORMAT_R32_UINT:
        return VK_FORMAT_R32_UINT;
    case SPV_REFLECT_FORMAT_R32_SINT:
        return VK_FORMAT_R32_SINT;
    case SPV_REFLECT_FORMAT_R32_SFLOAT:
        return VK_FORMAT_R32_SFLOAT;
    case SPV_REFLECT_FORMAT_R32G32_UINT:
        return VK_FORMAT_R32G32_UINT;
    case SPV_REFLECT_FORMAT_R32G32_SINT:
        return VK_FORMAT_R32G32_SINT;
    case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
        return VK_FORMAT_R32G32_SFLOAT;
    case SPV_REFLECT_FORMAT_R32G32B32_UINT:
        return VK_FORMAT_R32G32B32_UINT;
    case SPV_REFLECT_FORMAT_R32G32B32_SINT:
        return VK_FORMAT_R32G32B32_SINT;
    case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
        return VK_FORMAT_R32G32B32A32_UINT;
    case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
        return VK_FORMAT_R32G32B32A32_SINT;
    case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case SPV_REFLECT_FORMAT_R64_UINT:
        return VK_FORMAT_R64_UINT;
    case SPV_REFLECT_FORMAT_R64_SINT:
        return VK_FORMAT_R64_SINT;
    case SPV_REFLECT_FORMAT_R64_SFLOAT:
        return VK_FORMAT_R64_SFLOAT;
    case SPV_REFLECT_FORMAT_R64G64_UINT:
        return VK_FORMAT_R64G64_UINT;
    case SPV_REFLECT_FORMAT_R64G64_SINT:
        return VK_FORMAT_R64G64_SINT;
    case SPV_REFLECT_FORMAT_R64G64_SFLOAT:
        return VK_FORMAT_R64G64_SFLOAT;
    case SPV_REFLECT_FORMAT_R64G64B64_UINT:
        return VK_FORMAT_R64G64B64_UINT;
    case SPV_REFLECT_FORMAT_R64G64B64_SINT:
        return VK_FORMAT_R64G64B64_SINT;
    case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:
        return VK_FORMAT_R64G64B64_SFLOAT;
    case SPV_REFLECT_FORMAT_R64G64B64A64_UINT:
        return VK_FORMAT_R64G64B64A64_UINT;
    case SPV_REFLECT_FORMAT_R64G64B64A64_SINT:
        return VK_FORMAT_R64G64B64A64_SINT;
    case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT:
        return VK_FORMAT_R64G64B64A64_SFLOAT;

    default:
        return VK_FORMAT_UNDEFINED;
    }
}


// ---------- pipeline ----------
void vk::FVkShader::BuildPipeline(FVkPipeline& pipeline, const GetPipelineInfo& info,
                                  const std::shared_ptr<FVkPipelineLayout>& pipelineLayout) const
{
    assert(pipelineLayout);

    FGraphicsPipelineDesc desc{};
    desc.pushConstants = &m_PushConstants;
    desc.pVertexInputState = &m_VertexInput.createInfo;
    desc.topology = info.topology;
    desc.vertexShader = m_VertexShader.shaderModule;
    desc.fragmentShader = m_FragmentShader.shaderModule;
    desc.depthTestEnable = info.depthTestEnable;
    desc.depthWriteEnable = info.depthWriteEnable;
    desc.depthCompareOp = info.depthCompareOp;
    desc.samplesCount = info.samplesCount;
    desc.cullMode = info.cullMode;
    desc.frontFace = info.frontFace;
    desc.depthBiasEnable = info.depthBiasEnable;
    desc.depthBiasConstantFactor = info.depthBiasConstantFactor;
    desc.depthBiasClamp = info.depthBiasClamp;
    desc.depthBiasSlopeFactor = info.depthBiasSlopeFactor;
    desc.colorAttachmentCount = info.colorAttachmentCount;
    desc.colorFormat = info.colorFormat;
    desc.depthFormat = info.depthFormat;
    desc.vertexEntryPointName = m_VertexShader.entryPoint.c_str();
    desc.fragmentEntryPointName = m_FragmentShader.entryPoint.c_str();
    desc.shaderStages = &m_ShaderStages;
    desc.colorBlendAttachment.blendEnable = info.blendEnable;

    pipeline.Init(m_Device, desc, pipelineLayout);
}
