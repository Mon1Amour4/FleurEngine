#pragma once

#include <memory>
#include <functional>
#include <unordered_map>

#include "FVkPipeline.h"
#include "FVkPipelineLayout.h"
#include "FVkShader.h"

class FVkPipelineCache
{
public:
    FVkPipeline& Get(const vk::FVkShader& shader, const vk::GetPipelineInfo& info,
                     const std::shared_ptr<FVkPipelineLayout>& pipelineLayout);

private:
    struct Key
    {
        vk::GetPipelineInfo info;
        VkPipelineLayout layout{VK_NULL_HANDLE};
        const vk::FVkShader* shader{nullptr};

        bool operator==(const Key& rhs) const noexcept
        {
            return info == rhs.info && layout == rhs.layout && shader == rhs.shader;
        }
    };

    struct KeyHash
    {
        size_t operator()(const Key& key) const noexcept
        {
            size_t hash = std::hash<vk::GetPipelineInfo>{}(key.info);
            hash ^= std::hash<VkPipelineLayout>{}(key.layout) + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
            hash ^= std::hash<const vk::FVkShader*>{}(key.shader) + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
            return hash;
        }
    };

    std::unordered_map<Key, FVkPipeline, KeyHash> m_Pipelines;
};
