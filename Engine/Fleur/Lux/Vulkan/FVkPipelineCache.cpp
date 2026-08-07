#include "FVkPipelineCache.h"

#include <cassert>

FVkPipeline& FVkPipelineCache::Get(const vk::FVkShader& shader, const vk::GetPipelineInfo& info,
                                   const std::shared_ptr<FVkPipelineLayout>& pipelineLayout)
{
    assert(pipelineLayout);
    FVkPipeline& pipeline = m_Pipelines[Key{info, pipelineLayout->Get(), &shader}];
    if (!pipeline.GetPipelineLayout())
        shader.BuildPipeline(pipeline, info, pipelineLayout);
    return pipeline;
}
