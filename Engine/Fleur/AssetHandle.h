#pragma once

#include <cstdint>

namespace Fleur
{

struct AssetHandle
{
    uint32_t id = 0;
    uint32_t generation = 0;

    [[nodiscard]] bool IsValid() const
    {
        return id != 0;
    }
};

}  // namespace Fleur
