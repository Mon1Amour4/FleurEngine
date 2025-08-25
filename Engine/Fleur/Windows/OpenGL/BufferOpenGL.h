#pragma once

#include "Renderer/Buffer.h"

namespace Fleur::Graphics
{
enum RenderStage;

class BufferOpenGL final : public Buffer
{
public:
    virtual ~BufferOpenGL() override;

    uint32_t GetBufferID() const;
    virtual uint32_t UpdateSubDataImpl(const void* data, size_t size) override;
    virtual uint32_t NativeType() const override;

private:
    uint32_t m_Id;
    uint32_t m_BufferNativeType;

protected:
    friend class DeviceOpenGL;
    int native_usage(RenderStage& stage) const;
    int native_buffer_type(const EBufferType& type) const;
    BufferOpenGL(EBufferType type, RenderStage stage, size_t size_bytes);
};
}  // namespace Fleur::Graphics
