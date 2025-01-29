#pragma once
#include <cstdint>

namespace Fuego::Renderer
{
struct VertexLayout
{
public:
    enum DataType
    {
        FLOAT = 0,
        FLOAT_VEC2 = 1,
        FLOAT_VEC3 = 2,
        FLOAT_VEC4 = 3,
        INSIGNED_BYTE = 4,
        SHORT = 5
    };

    struct VertexAttribute
    {
        uint16_t index;
        uint8_t components_amount;
        DataType compontnts_type;
        uint16_t offset;
        bool is_enabled;
    };

    uint16_t layout_size;
    std::vector<VertexAttribute> attribs;

    VertexLayout();
    void AddAttribute(VertexAttribute attrib);
    void EnableAttribute(uint16_t attrib_index);
    void DisableAttribute(uint16_t attrib_index);
    uint32_t GetAPIDataType(DataType type);
};
}  // namespace Fuego::Renderer