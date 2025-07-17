#include "VertexLayout.h"

#if defined(FUEGO_PLATFORM_WIN)
#include "glad/gl.h"
#elif defined(FUEGO_PLATFORM_MACOS)
#endif
namespace Fuego::Graphics
{
// Layout:
VertexLayout::VertexLayout()
    : stride(0)
    , _it(nullptr)
{
    attribs.reserve(1);
}

uint32_t VertexLayout::GetAPIDataType(DataType type) const
{
#if defined(FUEGO_PLATFORM_WIN)
    switch (type)
    {
    case DataType::FLOAT:
        return GL_FLOAT;
    case DataType::FLOAT_VEC2:
        return GL_FLOAT_VEC2;
    case DataType::FLOAT_VEC3:
        return GL_FLOAT_VEC3;
    case DataType::FLOAT_VEC4:
        return GL_FLOAT_VEC4;
    case DataType::INSIGNED_BYTE:
        return GL_UNSIGNED_BYTE;
    case DataType::SHORT:
        return GL_SHORT;
    default:
        return 0;
    }
#elif defined(FUEGO_PLATFORM_MACOS)
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

uint32_t VertexLayout::GetSizeOfDataType(DataType type) const
{
    switch (type)
    {
    case DataType::FLOAT:
        return sizeof(float);
    case DataType::FLOAT_VEC2:
        return sizeof(float) * 2;
    case DataType::FLOAT_VEC3:
        return sizeof(float) * 3;
    case DataType::FLOAT_VEC4:
        return sizeof(float) * 4;
    case DataType::INSIGNED_BYTE:
        return sizeof(unsigned char);
    case DataType::SHORT:
        return sizeof(short);
    default:
        return 0;
    }
}

void VertexLayout::AddAttribute(VertexAttribute attrib)
{
    attrib.offset = stride;
    stride += GetSizeOfDataType(attrib.components_type) * attrib.components_amount;
    attribs.push_back(attrib);
}

void VertexLayout::EnableAttribute(uint16_t attrib_index)
{
    if (attrib_index < attribs.size())
    {
        attribs[attrib_index].is_enabled = true;
    }
}

void VertexLayout::DisableAttribute(uint16_t attrib_index)
{
    if (attrib_index < attribs.size())
    {
        attribs[attrib_index].is_enabled = false;
    }
}


// Attribute:
VertexLayout::VertexAttribute::VertexAttribute(uint16_t ind, uint8_t comp_amount, DataType comp_type, bool enabled)
    : index(ind)
    , components_amount(comp_amount)
    , components_type(comp_type)
    , is_enabled(enabled)
    , offset(0)
{
}


// Iterator:
VertexLayout::LayoutIterator::LayoutIterator(VertexLayout* master, VertexAttribute* attrib)
    : _master(master)
    , _attrib(attrib)
    , is_done(false)
    , index(0)
{
}

VertexLayout::LayoutIterator* VertexLayout::GetIteratorBegin()
{
    if (attribs.size() == 0)
    {
        _it->is_done = true;
        return nullptr;
    }
    _it = new LayoutIterator(this, &attribs[0]);
    _it->index = 0;
    return _it;
}

VertexLayout::LayoutIterator* VertexLayout::GetNextIterator()
{
    if (!_it)
        return nullptr;
    if (attribs.empty())
    {
        _it->is_done = true;
        return nullptr;
    }

    if (_it->index + 1 < attribs.size())
    {
        _it->_attrib++;
        _it->index++;
        return _it;
    }
    else
    {
        _it->is_done = true;
    }

    return nullptr;
}

bool VertexLayout::LayoutIterator::IsDone()
{
    return is_done;
}

void VertexLayout::ReleaseIterator()
{
    delete _it;
    _it = nullptr;
}

}  // namespace Fuego::Graphics
