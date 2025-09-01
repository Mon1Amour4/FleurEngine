#pragma once

#include "Renderer/Buffer.h"

namespace Fleur::Graphics
{
enum ERenderStage;

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
    int NativeUsage(ERenderStage& stage) const;
    int NativeBufferType(const EBufferType& type) const;
    BufferOpenGL(EBufferType type, ERenderStage stage, size_t sizeBytes);
};
}  // namespace Fleur::Graphics
