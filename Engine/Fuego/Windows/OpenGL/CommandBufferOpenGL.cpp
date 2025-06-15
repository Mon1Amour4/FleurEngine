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

namespace Fuego::Graphics
{
CommandBufferOpenGL::CommandBufferOpenGL()
    : _mainVsShader(-1), _pixelShader(-1), _isLinked(false), _isDataAllocated(false), _texture(0), _isFree(true)
{
    glGenBuffers(1, &_ebo);
    glGenVertexArrays(1, &_vao);
}

CommandBufferOpenGL::~CommandBufferOpenGL()
{
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_ebo);

    for (size_t i = 0; i < push_debug_group_commands; i++)
    {
        PopDebugGroup();
    }
}

void CommandBufferOpenGL::BeginRecording()
{
    _isFree = false;
    // Clear();
}

void CommandBufferOpenGL::EndRecording()
{
    // Temporary
    // glDeleteTextures(1, &_texture);
    glBindVertexArray(0);
}

void CommandBufferOpenGL::Submit()
{
    // TODO Execute gl commands from commands queue
    _isFree = true;
}

void CommandBufferOpenGL::BindRenderTarget(const Surface& texture)
{
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CommandBufferOpenGL::BindVertexBuffer(std::unique_ptr<Buffer> vertexBuffer, VertexLayout layout)
{
    vertex_global_buffer = std::move(vertexBuffer);
    auto buff = static_cast<const BufferOpenGL*>(vertex_global_buffer.get());

    glBindVertexArray(_vao);
    glBindBuffer(vertex_global_buffer->NativeType(), buff->GetBufferID());

    VertexLayout::LayoutIterator* it;
    for (it = layout.GetIteratorBegin(); it && !it->IsDone(); it = layout.GetNextIterator())
    {
        glVertexAttribPointer((GLuint)it->GetIndex(), (GLint)it->GetComponentsAmount(), it->GetAPIDatatype(), GL_FALSE,
                              (GLsizei)layout.GetLayoutSize(), (void*)it->GetOffset());
        if (it->GetIsEnabled())
            glEnableVertexAttribArray(it->GetIndex());
    }
    glBindVertexArray(0);
    glBindBuffer(vertex_global_buffer->NativeType(), 0);
}

void CommandBufferOpenGL::BindIndexBuffer(std::unique_ptr<Buffer> buffer)
{
    index_global_buffer = std::move(buffer);
    auto buff = static_cast<const BufferOpenGL*>(index_global_buffer.get());

    glBindVertexArray(_vao);
    glBindBuffer(index_global_buffer->NativeType(), buff->GetBufferID());

    glBindVertexArray(0);
}

uint32_t CommandBufferOpenGL::UpdateBufferSubDataImpl(Buffer::BufferType type, const void* data, size_t size_bytes)
{
    if (type == Buffer::BufferType::Vertex)
        return vertex_global_buffer->UpdateSubData(data, size_bytes);
    else
        return index_global_buffer->UpdateSubData(data, size_bytes);
}

void CommandBufferOpenGL::BindTexture(Texture* texture)
{
    TextureOpenGL& text_gl = static_cast<TextureOpenGL&>(*texture);

    glBindTextureUnit(text_gl.GetTextureUnit(), text_gl.GetTextureID());
}

void CommandBufferOpenGL::Draw(uint32_t vertexCount)
{
    // glUseProgram(_programID);
    glBindVertexArray(_vao);

    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}

void CommandBufferOpenGL::IndexedDraw(uint32_t index_count, size_t index_offset_bytes, uint32_t base_vertex)
{
    glBindVertexArray(_vao);
    auto ptr = reinterpret_cast<void*>(index_offset_bytes);
    glDrawElementsBaseVertex(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, ptr, base_vertex);
}

void CommandBufferOpenGL::Clear()
{
    glClearColor(1.f, 1.f, 1.f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void CommandBufferOpenGL::PushDebugGroup(uint32_t id, const char* message)
{
    GLint max_length = 0;
    glGetIntegerv(GL_MAX_LABEL_LENGTH, &max_length);

    if (!message || *message == '\n')
        FU_CORE_INFO("[Render Marker] PushDebugGroup: message is empty");

    size_t length = strlen(message);
    if (length > max_length)
        FU_CORE_INFO("[Render Marker] PushDebugGroup: message is too long");

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, id, length, message);
    push_debug_group_commands++;
}

void CommandBufferOpenGL::PopDebugGroup()
{
    glPopDebugGroup();
    push_debug_group_commands--;
}

void CommandBufferOpenGL::SetLabel(ObjectLabel id, uint32_t name, const char* message)
{
    GLint max_length = 0;
    glGetIntegerv(GL_MAX_LABEL_LENGTH, &max_length);

    if (!message || *message == '\n')
        FU_CORE_INFO("[Render Marker] PushDebugGroup: message is empty");

    size_t length = strlen(message);
    if (length > max_length)
        FU_CORE_INFO("[Render Marker] PushDebugGroup: message is too long");

    GLenum identifier = GL_BUFFER;
    switch (id)
    {
        case Fuego::Graphics::CommandBuffer::LABEL_BUFFER:
            identifier = GL_BUFFER;
            break;
        case Fuego::Graphics::CommandBuffer::LABEL_SHADER:
            identifier = GL_SHADER;
            break;
        case Fuego::Graphics::CommandBuffer::LABEL_TEXTURE:
            identifier = GL_TEXTURE;
            break;
    }
    glObjectLabel(identifier, name, length, message);
}

void CommandBufferOpenGL::BindShaderObject(std::shared_ptr<Fuego::Graphics::ShaderObject> shader)
{
    shader_object = shader;
}

void CommandBufferOpenGL::BindDescriptorSet(const DescriptorBuffer& descriptorSet, int setIndex)
{
    UNUSED(descriptorSet);
    UNUSED(setIndex);
    FU_CORE_INFO("[OpenGL unused function: BindDescriptorSet]");
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

}  // namespace Fuego::Graphics
