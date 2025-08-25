#include "VertexLayout.h"

#if defined(FLEUR_PLATFORM_WIN)
#include "glad/gl.h"
#elif defined(FLEUR_PLATFORM_MACOS)
#endif
namespace Fleur::Graphics
{
// Layout:
VertexLayout::VertexLayout()
    : m_Stride(0)
    , m_It(nullptr)
{
    m_Attribs.reserve(1);
}

uint32_t VertexLayout::GetAPIDataType(EDataType type) const
{
#if defined(FLEUR_PLATFORM_WIN)
    switch (type)
    {
    case EDataType::FLOAT:
        return GL_FLOAT;
    case EDataType::FLOAT_VEC2:
        return GL_FLOAT_VEC2;
    case EDataType::FLOAT_VEC3:
        return GL_FLOAT_VEC3;
    case EDataType::FLOAT_VEC4:
        return GL_FLOAT_VEC4;
    case EDataType::INSIGNED_BYTE:
        return GL_UNSIGNED_BYTE;
    case EDataType::SHORT:
        return GL_SHORT;
    default:
        return 0;
    }
#elif defined(FLEUR_PLATFORM_MACOS)
    switch (type)
    {
    case DataType::FLOAT:
    case DataType::FLOAT_VEC2:
    case DataType::FLOAT_VEC3:
    case DataType::FLOAT_VEC4:
    case DataType::INSIGNED_BYTE:
    case DataType::SHORT:
    default:
        return 0;
    }
#endif
}

uint32_t VertexLayout::GetSizeOfDataType(EDataType type) const
{
    switch (type)
    {
    case EDataType::FLOAT:
        return sizeof(float);
    case EDataType::FLOAT_VEC2:
        return sizeof(float) * 2;
    case EDataType::FLOAT_VEC3:
        return sizeof(float) * 3;
    case EDataType::FLOAT_VEC4:
        return sizeof(float) * 4;
    case EDataType::INSIGNED_BYTE:
        return sizeof(unsigned char);
    case EDataType::SHORT:
        return sizeof(short);
    default:
        return 0;
    }
}

void VertexLayout::AddAttribute(VertexAttribute attrib)
{
    attrib.Offset = m_Stride;
    m_Stride += GetSizeOfDataType(attrib.ComponentsType) * attrib.ComponentsAmount;
    m_Attribs.push_back(attrib);
}

void VertexLayout::EnableAttribute(uint16_t attrib_index)
{
    if (attrib_index < m_Attribs.size())
    {
        m_Attribs[attrib_index].m_IsEnabled = true;
    }
}

void VertexLayout::DisableAttribute(uint16_t attrib_index)
{
    if (attrib_index < m_Attribs.size())
    {
        m_Attribs[attrib_index].m_IsEnabled = false;
    }
}


// Attribute:
VertexLayout::VertexAttribute::VertexAttribute(uint16_t ind, uint8_t comp_amount, EDataType comp_type, bool enabled)
    : Index(ind)
    , ComponentsAmount(comp_amount)
    , ComponentsType(comp_type)
    , m_IsEnabled(enabled)
    , Offset(0)
{
}


// Iterator:
VertexLayout::LayoutIterator::LayoutIterator(VertexLayout* master, VertexAttribute* attrib)
    : m_Master(master)
    , m_Attrib(attrib)
    , m_IsDone(false)
    , m_Idx(0)
{
}

VertexLayout::LayoutIterator* VertexLayout::GetIteratorBegin()
{
    if (m_Attribs.size() == 0)
    {
        m_It->m_IsDone = true;
        return nullptr;
    }
    m_It = new LayoutIterator(this, &m_Attribs[0]);
    m_It->m_Idx = 0;
    return m_It;
}

VertexLayout::LayoutIterator* VertexLayout::GetNextIterator()
{
    if (!m_It)
        return nullptr;
    if (m_Attribs.empty())
    {
        m_It->m_IsDone = true;
        return nullptr;
    }

    if (m_It->m_Idx + 1 < m_Attribs.size())
    {
        m_It->m_Attrib++;
        m_It->m_Idx++;
        return m_It;
    }
    else
    {
        m_It->m_IsDone = true;
    }

    return nullptr;
}

bool VertexLayout::LayoutIterator::IsDone()
{
    return m_IsDone;
}

void VertexLayout::ReleaseIterator()
{
    delete m_It;
    m_It = nullptr;
}

}  // namespace Fleur::Graphics
