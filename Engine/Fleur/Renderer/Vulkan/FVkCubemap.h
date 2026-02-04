#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "FVkTexture.h"

class FVkCubemap
{
public:
    FVkCubemap() = default;
    ~FVkCubemap();

    inline FVkTexture& GetFaceTexture(uint32_t idx)
    {
        return m_Textures[idx];
    }

private:
    VkDevice m_Device;

    std::vector<FVkTexture> m_Textures{6};
};
