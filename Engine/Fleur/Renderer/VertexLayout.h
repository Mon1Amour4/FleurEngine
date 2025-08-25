#pragma once
#include <cstdint>

namespace Fleur::Graphics
{
struct VertexLayout
{
public:
    enum EDataType
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
        VertexAttribute(uint16_t ind, uint8_t compAmount, EDataType compType, bool enabled);
        uint16_t Index;
        uint8_t ComponentsAmount;
        EDataType ComponentsType;
        uint16_t Offset;
        bool m_IsEnabled;
    };

    struct LayoutIterator
    {
    public:
        LayoutIterator(VertexLayout* master, VertexAttribute* attrib);

        inline uint16_t GetIndex()
        {
            return m_Attrib->Index;
        }

        inline uint8_t GetComponentsAmount()
        {
            return m_Attrib->ComponentsAmount;
        }

        inline uint32_t GetAPIDatatype()
        {
            return m_Master->GetAPIDataType(m_Attrib->ComponentsType);
        }

        inline uint16_t GetOffset()
        {
            return m_Attrib->Offset;
        }

        inline bool GetIsEnabled()
        {
            return m_Attrib->m_IsEnabled;
        }

        bool IsDone();

    private:
        friend struct VertexLayout;
        VertexAttribute* m_Attrib;
        VertexLayout* m_Master;
        bool m_IsDone;
        uint16_t m_Idx;
    };

    VertexLayout();
    ~VertexLayout() = default;

    void AddAttribute(VertexAttribute attrib);

    void EnableAttribute(uint16_t attrib_index);

    void DisableAttribute(uint16_t attrib_index);

    uint32_t GetAPIDataType(EDataType type) const;

    uint32_t GetSizeOfDataType(EDataType type) const;

    inline uint32_t Stride() const
    {
        return m_Stride;
    }

    LayoutIterator* GetIteratorBegin();

    LayoutIterator* GetNextIterator();

    void ReleaseIterator();

private:
    std::vector<VertexAttribute> m_Attribs;
    uint32_t m_Stride;
    LayoutIterator* m_It;
};
}  // namespace Fleur::Graphics
