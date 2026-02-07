#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "FVkTexture.h"

class FVkCubemap
{
public:
    FVkCubemap() = default;
    ~FVkCubemap();

    inline FVkTexture* GetCubemapTexture()
    {
        return &m_Texture;
    }

private:
    VkDevice m_Device;

    FVkTexture m_Texture;
};
