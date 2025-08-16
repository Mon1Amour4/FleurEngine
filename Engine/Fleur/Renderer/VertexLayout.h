#pragma once
#include <cstdint>

namespace Fleur::Graphics
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
        VertexAttribute(uint16_t ind, uint8_t comp_amount, DataType comp_type, bool enabled);
        uint16_t index;
        uint8_t components_amount;
        DataType components_type;
        uint16_t offset;
        bool is_enabled;
    };

    struct LayoutIterator
    {
    public:
        LayoutIterator(VertexLayout* master, VertexAttribute* attrib);

        inline uint16_t GetIndex()
        {
            return _attrib->index;
        }

        inline uint8_t GetComponentsAmount()
        {
            return _attrib->components_amount;
        }

        inline uint32_t GetAPIDatatype()
        {
            return _master->GetAPIDataType(_attrib->components_type);
        }

        inline uint16_t GetOffset()
        {
            return _attrib->offset;
        }

        inline bool GetIsEnabled()
        {
            return _attrib->is_enabled;
        }

        bool IsDone();

    private:
        friend struct VertexLayout;
        VertexAttribute* _attrib;
        VertexLayout* _master;
        bool is_done;
        uint16_t index;
    };

    VertexLayout();
    ~VertexLayout() = default;

    void AddAttribute(VertexAttribute attrib);

    void EnableAttribute(uint16_t attrib_index);

    void DisableAttribute(uint16_t attrib_index);

    uint32_t GetAPIDataType(DataType type) const;

    uint32_t GetSizeOfDataType(DataType type) const;

    inline uint32_t Stride() const
    {
        return stride;
    }

    LayoutIterator* GetIteratorBegin();

    LayoutIterator* GetNextIterator();

    void ReleaseIterator();

private:
    std::vector<VertexAttribute> attribs;
    uint32_t stride;
    LayoutIterator* _it;
};
}  // namespace Fleur::Graphics
