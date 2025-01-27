#include "CommandBufferOpenGL.h"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include "BufferOpenGL.h"
#include "Renderer.h"
#include "Renderer/Surface.h"
#include "ShaderObjectOpenGL.h"
#include "ShaderOpenGL.h"
#include "glad/gl.h"

namespace Fuego::Renderer
{
CommandBufferOpenGL::CommandBufferOpenGL()
    : _isFree(true)
    , _mainVsShader(-1)
    , _pixelShader(-1)
    , _isLinked(false)
    , _vao(0)
    , _ebo(0)
    , _isDataAllocated(false)
{
    glGenBuffers(1, &_ebo);
    glGenVertexArrays(1, &_vao);
}

CommandBufferOpenGL::~CommandBufferOpenGL()
{
    glDeleteBuffers(1, &_vao);
    glDeleteBuffers(1, &_ebo);
}

void CommandBufferOpenGL::BeginRecording()
{
    _isFree = false;
    Clear();
}

void CommandBufferOpenGL::EndRecording()
{
    // Temporary
    glDeleteTextures(1, &_texture);
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

void CommandBufferOpenGL::BindVertexBuffer(const Buffer& vertexBuffer)
{
    glBindVertexArray(_vao);

    const BufferOpenGL& buff = static_cast<const BufferOpenGL&>(vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, buff.GetBufferID());

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Renderer::VertexData), (void*)offsetof(Renderer::VertexData, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Renderer::VertexData), (void*)offsetof(Renderer::VertexData, textcoord));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Renderer::VertexData), (void*)offsetof(Renderer::VertexData, normal));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CommandBufferOpenGL::BindIndexBuffer(uint32_t indices[], uint32_t size)
{
    glBindVertexArray(_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    if (!_isDataAllocated)
    {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_DYNAMIC_DRAW);
        _isDataAllocated = true;
    }
    else
    {
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size, indices);
    }
    glBindVertexArray(0);
}

void CommandBufferOpenGL::BindTexture(unsigned char* data, int w, int h)
{
    GLenum textureTarget = GL_TEXTURE_2D;
    glBindTexture(GL_TEXTURE_2D, 0);
    glGenTextures(1, &_texture);
    glBindTexture(textureTarget, _texture);
    glTexImage2D(textureTarget, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    // glBindTexture(textureTarget, texture);

    // Configuration of minification/Magnification
    glTexParameterf(textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // glBindTexture(texture, 0);
    glActiveTexture(GL_TEXTURE0);
    // glBindTexture(_textureTarget, _texture);
}

void CommandBufferOpenGL::Draw(uint32_t vertexCount)
{
    // glUseProgram(_programID);
    glBindVertexArray(_vao);

    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}

void CommandBufferOpenGL::IndexedDraw(uint32_t vertexCount)
{
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void CommandBufferOpenGL::Clear()
{
    glClearColor(1.f, 1.f, 1.f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void CommandBufferOpenGL::BindShaderObject(const ShaderObject& obj)
{
    const ShaderObjectOpenGL& obj_gl = static_cast<const ShaderObjectOpenGL&>(obj);
    obj_gl.Use();
}

void CommandBufferOpenGL::BindDescriptorSet(const DescriptorBuffer& descriptorSet, int setIndex)
{
    UNUSED(descriptorSet);
    UNUSED(setIndex);
    FU_CORE_INFO("[OpenGL unused function: BindDescriptorSet]");
}

}  // namespace Fuego::Renderer
