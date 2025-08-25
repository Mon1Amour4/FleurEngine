#include "CommandBufferOpenGL.h"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include "../../Renderer/Graphics.hpp"
#include "BufferOpenGL.h"
#include "Renderer.h"
#include "ShaderObjectOpenGL.h"
#include "TextureOpenGL.h"
#include "VertexLayout.h"
#include "glad/gl.h"

namespace Fleur::Graphics
{
CommandBufferOpenGL::CommandBufferOpenGL(DepthStencilDescriptor desc)
    : CommandBuffer(desc)
    , _mainVsShader(-1)
    , _pixelShader(-1)
    , _isLinked(false)
    , _isDataAllocated(false)
    , _texture(0)
    , _isFree(true)
{
    glCreateVertexArrays(1, &_vao);
}

CommandBufferOpenGL::~CommandBufferOpenGL()
{
    glDeleteVertexArrays(1, &_vao);

    for (size_t i = 0; i < m_PushDebugGroupCommands; i++)
    {
        PopDebugGroup();
    }
}

void CommandBufferOpenGL::BeginRecording()
{
    if (m_Descriptor.death_test)
        glDepthMask(true);
    else
        glDepthMask(false);

    glDepthFunc(get_death_func_op(m_Descriptor.operation));

    glBindVertexArray(_vao);
    _isFree = false;
}

void CommandBufferOpenGL::EndRecording()
{
}

void CommandBufferOpenGL::Submit()
{
    _isFree = true;
}

void CommandBufferOpenGL::BindRenderTarget(const Framebuffer& fbo, FramebufferRWOperation rw)
{
    if (rw == FramebufferRWOperation::READ_ONLY)
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<const FramebufferOpenGL&>(fbo).ID());
    else if (rw == FramebufferRWOperation::WRITE_ONLY)
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<const FramebufferOpenGL&>(fbo).ID());
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<const FramebufferOpenGL&>(fbo).ID());
        glViewport(0, 0, fbo.Width(), fbo.Height());
    }
}

void CommandBufferOpenGL::BindVertexBuffer(std::unique_ptr<Buffer> vertexBuffer, VertexLayout layout)
{
    m_VertexGlobalBuffer = std::move(vertexBuffer);
    auto buff = static_cast<const BufferOpenGL*>(m_VertexGlobalBuffer.get());

    glVertexArrayVertexBuffer(_vao, 0, buff->GetBufferID(), 0, layout.Stride());

    VertexLayout::LayoutIterator* it;
    for (it = layout.GetIteratorBegin(); it && !it->IsDone(); it = layout.GetNextIterator())
    {
        GLuint index = static_cast<GLuint>(it->GetIndex());
        glVertexArrayAttribFormat(_vao, index, it->GetComponentsAmount(), it->GetAPIDatatype(), GL_FALSE, static_cast<GLuint>(it->GetOffset()));
        glVertexArrayAttribBinding(_vao, index, 0);
        if (it->GetIsEnabled())
            glEnableVertexArrayAttrib(_vao, index);
    }
}

void CommandBufferOpenGL::BindIndexBuffer(std::unique_ptr<Buffer> buffer)
{
    m_IndexGlobalBuffer = std::move(buffer);
    auto buff = static_cast<const BufferOpenGL*>(m_IndexGlobalBuffer.get());

    glVertexArrayElementBuffer(_vao, buff->GetBufferID());
}

uint32_t CommandBufferOpenGL::UpdateBufferSubDataImpl(Buffer::EBufferType type, const void* data, size_t size_bytes)
{
    if (type == Buffer::EBufferType::Vertex)
        return m_VertexGlobalBuffer->UpdateSubData(data, size_bytes);
    else
        return m_IndexGlobalBuffer->UpdateSubData(data, size_bytes);
}

void CommandBufferOpenGL::BindTexture(Texture* texture)
{
    TextureOpenGL& text_gl = static_cast<TextureOpenGL&>(*texture);

    glBindTextureUnit(text_gl.GetTextureUnit(), *text_gl.GetTextureID());
}

void CommandBufferOpenGL::Draw(uint32_t vertexCount)
{
    glBindVertexArray(_vao);

    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}

void CommandBufferOpenGL::IndexedDraw(uint32_t index_count, size_t index_offset_bytes, uint32_t base_vertex)
{
    glDrawElementsBaseVertex(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, reinterpret_cast<void*>(index_offset_bytes), base_vertex);
}

void CommandBufferOpenGL::PushDebugGroup(uint32_t id, const char* message)
{
    if (!message || !*message || *message == '\n')
    {
        FL_CORE_INFO("[Render Marker] PushDebugGroup: message is empty or invalid");
        return;
    }

    GLsizei length = static_cast<GLsizei>(std::strlen(message));
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, id, length, message);

    m_PushDebugGroupCommands++;
}

void CommandBufferOpenGL::PopDebugGroup()
{
    glPopDebugGroup();
    m_PushDebugGroupCommands--;
}

void CommandBufferOpenGL::SetLabel(ObjectLabel id, uint32_t name, const char* message)
{
    if (!message || !*message || *message == '\n')
    {
        FL_CORE_INFO("[Render Marker] SetLabel: message is empty or invalid");
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

void CommandBufferOpenGL::BindShaderObject(std::shared_ptr<Fleur::Graphics::ShaderObject> shader)
{
    m_ShaderObject = shader;
}

void CommandBufferOpenGL::BindDescriptorSet(const DescriptorBuffer& descriptorSet, int setIndex)
{
    UNUSED(descriptorSet);
    UNUSED(setIndex);
    FL_CORE_INFO("[OpenGL unused function: BindDescriptorSet]");
}

int CommandBufferOpenGL::ConvertUsage(RenderStage& stage) const
{
    switch (stage)
    {
    case STATIC_GEOMETRY:
        return GL_STATIC_DRAW;
    case DYNAMIC_DRAW:
        return GL_DYNAMIC_DRAW;
    }
}

uint32_t CommandBufferOpenGL::get_death_func_op(EDepthTestOperation op) const
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

}  // namespace Fleur::Graphics
