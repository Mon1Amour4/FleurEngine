#include "VertexLayout.h"

// Layout:
Fleur::Graphics::VertexLayout::VertexLayout()
    : m_Stride(0)
    , m_It(nullptr)
{
    m_Attribs.reserve(1);
}

uint32_t Fleur::Graphics::VertexLayout::GetAPIDataType(EDataType type) const
{
#if defined(FLEUR_PLATFORM_WIN)

    // TODO Get rid of this
    switch (type)
    {
    case EDataType::FLOAT:
        return 0x1406;
    case EDataType::FLOAT_VEC2:
        return 0x8B50;
    case EDataType::FLOAT_VEC3:
        return 0x8B51;
    case EDataType::FLOAT_VEC4:
        return 0x8B52;
    case EDataType::INSIGNED_BYTE:
        return 0x1401;
    case EDataType::SHORT:
        return 0x1402;
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

uint16_t Fleur::Graphics::VertexLayout::GetSizeOfDataType(EDataType type) const
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

void Fleur::Graphics::VertexLayout::AddAttribute(VertexAttribute attrib)
{
    attrib.Offset = m_Stride;
    m_Stride += GetSizeOfDataType(attrib.ComponentsType) * attrib.ComponentsAmount;
    m_Attribs.push_back(attrib);
}

void Fleur::Graphics::VertexLayout::EnableAttribute(uint16_t attribIndex)
{
    if (attribIndex < m_Attribs.size())
    {
        m_Attribs[attribIndex].m_IsEnabled = true;
    }
}

void Fleur::Graphics::VertexLayout::DisableAttribute(uint16_t attribIndex)
{
    if (attribIndex < m_Attribs.size())
    {
        m_Attribs[attribIndex].m_IsEnabled = false;
    }
}


// Attribute:
Fleur::Graphics::VertexLayout::VertexAttribute::VertexAttribute(uint16_t ind, uint8_t compAmount, EDataType compType, bool enabled)
    : Index(ind)
    , ComponentsAmount(compAmount)
    , ComponentsType(compType)
    , m_IsEnabled(enabled)
    , Offset(0)
{
}


// Iterator:
Fleur::Graphics::VertexLayout::LayoutIterator::LayoutIterator(VertexLayout* master, VertexAttribute* attrib)
    : m_Master(master)
    , m_Attrib(attrib)
    , m_IsDone(false)
    , m_Idx(0)
{
}

Fleur::Graphics::VertexLayout::LayoutIterator* Fleur::Graphics::VertexLayout::GetIteratorBegin()
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

Fleur::Graphics::VertexLayout::LayoutIterator* Fleur::Graphics::VertexLayout::GetNextIterator()
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

bool Fleur::Graphics::VertexLayout::LayoutIterator::IsDone()
{
    return m_IsDone;
}

void Fleur::Graphics::VertexLayout::ReleaseIterator()
{
    delete m_It;
    m_It = nullptr;
}
