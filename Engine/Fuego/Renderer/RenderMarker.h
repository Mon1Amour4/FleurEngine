#pragma once

namespace Fuego::Renderer::Debug
{
class RenderMarker
{
public:
    virtual ~RenderMarker() = 0;


private:
    RenderMarker();
};

struct DebugGroup
{
public:
    DebugGroup();

private:
    virtual ~DebugGroup() = 0;

    virtual void Push(uint32_t id, uint16_t object_name, uint16_t label_length, const char* label) = 0;
    virtual void Pop() = 0;
};

struct ObjectLabel
{
public:
    virtual ~ObjectLabel() = 0;

    virtual void SetLabel(uint32_t id, uint16_t object_name, uint16_t label_length, const char* label) = 0;

private:
};
}  // namespace Fuego::Renderer::Debug