#include "CommandBufferOpenGL.h"

#include <glad/wgl.h>

#include "BufferOpenGL.h"
#include "Renderer.h"
#include "Renderer/Graphics.hpp"
#include "ShaderObjectOpenGL.h"
#include "TextureOpenGL.h"
#include "VertexLayout.h"

Fleur::Graphics::CommandBufferOpenGL::CommandBufferOpenGL(DepthStencilDescriptor descriptor)
    : CommandBuffer(descriptor)
    , m_MainVsShader(-1)
    , m_PixelShader(-1)
    , m_IsLinked(false)
    , m_IsDataAllocated(false)
    , m_Texture(0)
    , m_IsFree(true)
{
    glCreateVertexArrays(1, &m_VAO);
}

Fleur::Graphics::CommandBufferOpenGL::~CommandBufferOpenGL()
{
    glDeleteVertexArrays(1, &m_VAO);

    for (size_t i = 0; i < m_PushDebugGroupCommands; i++)
    {
        PopDebugGroup();
    }
}

void Fleur::Graphics::CommandBufferOpenGL::BeginRecording()
{
    if (m_Descriptor.death_test)
        glDepthMask(true);
    else
        glDepthMask(false);

    glDepthFunc(GetDeathFuncOp(m_Descriptor.operation));

    glBindVertexArray(m_VAO);
    m_IsFree = false;
}

void Fleur::Graphics::CommandBufferOpenGL::EndRecording()
{
}

void Fleur::Graphics::CommandBufferOpenGL::Submit()
{
    m_IsFree = true;
}

void Fleur::Graphics::CommandBufferOpenGL::BindRenderTarget(const Framebuffer& fbo, EFramebufferRWOperation rw)
{
    if (rw == EFramebufferRWOperation::READ_ONLY)
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<const FramebufferOpenGL&>(fbo).ID());
    else if (rw == EFramebufferRWOperation::WRITE_ONLY)
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<const FramebufferOpenGL&>(fbo).ID());
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<const FramebufferOpenGL&>(fbo).ID());
        glViewport(0, 0, fbo.Width(), fbo.Height());
    }
}

void Fleur::Graphics::CommandBufferOpenGL::BindVertexBuffer(std::unique_ptr<Buffer> vertexBuffer, VertexLayout layout)
{
    m_VertexGlobalBuffer = std::move(vertexBuffer);
    auto buff = static_cast<const BufferOpenGL*>(m_VertexGlobalBuffer.get());

    glVertexArrayVertexBuffer(m_VAO, 0, buff->GetBufferID(), 0, layout.Stride());

    VertexLayout::LayoutIterator* it;
    for (it = layout.GetIteratorBegin(); it && !it->IsDone(); it = layout.GetNextIterator())
    {
        GLuint index = static_cast<GLuint>(it->GetIndex());
        glVertexArrayAttribFormat(m_VAO, index, it->GetComponentsAmount(), it->GetAPIDatatype(), GL_FALSE, static_cast<GLuint>(it->GetOffset()));
        glVertexArrayAttribBinding(m_VAO, index, 0);
        if (it->GetIsEnabled())
            glEnableVertexArrayAttrib(m_VAO, index);
    }
}

void Fleur::Graphics::CommandBufferOpenGL::BindIndexBuffer(std::unique_ptr<Buffer> buffer)
{
    m_IndexGlobalBuffer = std::move(buffer);
    auto buff = static_cast<const BufferOpenGL*>(m_IndexGlobalBuffer.get());

    glVertexArrayElementBuffer(m_VAO, buff->GetBufferID());
}

size_t Fleur::Graphics::CommandBufferOpenGL::UpdateBufferSubDataImpl(Buffer::EBufferType type, const void* data, size_t sizeBytes)
{
    if (type == Buffer::EBufferType::Vertex)
        return m_VertexGlobalBuffer->UpdateSubData(data, sizeBytes);
    else
        return m_IndexGlobalBuffer->UpdateSubData(data, sizeBytes);
}

void Fleur::Graphics::CommandBufferOpenGL::BindTexture(Texture* texture)
{
    TextureOpenGL& textureGL = static_cast<TextureOpenGL&>(*texture);

    glBindTextureUnit(textureGL.GetTextureUnit(), *textureGL.GetTextureID());
}

void Fleur::Graphics::CommandBufferOpenGL::Draw(uint32_t vertexCount)
{
    glBindVertexArray(m_VAO);

    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}

void Fleur::Graphics::CommandBufferOpenGL::IndexedDraw(uint32_t indexCount, size_t indexOffsetBytes, uint32_t baseVertex)
{
    glDrawElementsBaseVertex(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(indexOffsetBytes), baseVertex);
}

void Fleur::Graphics::CommandBufferOpenGL::PushDebugGroup(uint32_t id, const char* message)
{
    if (!message || !*message || *message == '\n')
    {
        //FL_CORE_INFO("[Render Marker] PushDebugGroup: message is empty or invalid");
        return;
    }

    GLsizei length = static_cast<GLsizei>(std::strlen(message));
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, id, length, message);

    m_PushDebugGroupCommands++;
}

void Fleur::Graphics::CommandBufferOpenGL::PopDebugGroup()
{
    glPopDebugGroup();
    m_PushDebugGroupCommands--;
}

void Fleur::Graphics::CommandBufferOpenGL::SetLabel(EObjectLabel id, uint32_t name, const char* message)
{
    if (!message || !*message || *message == '\n')
    {
       // FL_CORE_INFO("[Render Marker] SetLabel: message is empty or invalid");
        return;
    }

    const GLsizei length = static_cast<GLsizei>(std::strlen(message));

    GLenum identifier = GL_BUFFER;
    switch (id)
    {
    case Fleur::Graphics::CommandBuffer::LABEL_BUFFER:
        identifier = GL_BUFFER;
        break;
    case Fleur::Graphics::CommandBuffer::LABEL_SHADER:
        identifier = GL_SHADER;
        break;
    case Fleur::Graphics::CommandBuffer::LABEL_TEXTURE:
        identifier = GL_TEXTURE;
        break;
    default:
        FL_CORE_WARN("SetLabel: Unknown object label type");
        return;
    }

    glObjectLabel(identifier, name, length, message);
}

void Fleur::Graphics::CommandBufferOpenGL::BindShaderObject(std::shared_ptr<Fleur::Graphics::ShaderObject> shader)
{
    m_ShaderObject = shader;
}

void Fleur::Graphics::CommandBufferOpenGL::BindDescriptorSet(const DescriptorBuffer& descriptorSet, int setIndex)
{
    UNUSED(descriptorSet);
    UNUSED(setIndex);
    FL_CORE_INFO("[OpenGL unused function: BindDescriptorSet]");
}

int Fleur::Graphics::CommandBufferOpenGL::ConvertUsage(ERenderStage& stage) const
{
    switch (stage)
    {
    case STATIC_GEOMETRY:
        return GL_STATIC_DRAW;
    case DYNAMIC_DRAW:
        return GL_DYNAMIC_DRAW;
    }
}

uint32_t Fleur::Graphics::CommandBufferOpenGL::GetDeathFuncOp(EDepthTestOperation op) const
{
    switch (op)
    {
    case Fleur::Graphics::EDepthTestOperation::NEVER:
        return GL_NEVER;
    case Fleur::Graphics::EDepthTestOperation::LESS:
        return GL_LESS;
    case Fleur::Graphics::EDepthTestOperation::LESS_OR_EQUAL:
        return GL_LEQUAL;
    case Fleur::Graphics::EDepthTestOperation::GREATER:
        return GL_GREATER;
    case Fleur::Graphics::EDepthTestOperation::EQUAL:
        return GL_EQUAL;
    case Fleur::Graphics::EDepthTestOperation::NOT_EQUAL:
        return GL_NOTEQUAL;
    case Fleur::Graphics::EDepthTestOperation::GREATER_OR_EQUAL:
        return GL_GEQUAL;
    case Fleur::Graphics::EDepthTestOperation::ALWAYS:
        return GL_ALWAYS;
    default:
        return GL_LESS;
    }
}
