#include "Renderer.h"

#include <span>

#include "Renderer.h"

namespace Fuego::Renderer
{

ShaderObject* shader_object;

uint32_t Renderer::MAX_TEXTURES_COUNT = 0;

Renderer::Renderer()
    : show_wireframe(false)
    , _camera(std::unique_ptr<Camera>(new Camera()))
    , current_shader_obj(nullptr)
{
    _camera->Activate();

    _device = Device::CreateDevice();
    _commandQueue = _device->CreateCommandQueue();

    // Temporary: we're creating surface for main Application window
    _surface = _device->CreateSurface(Fuego::Application::Get().GetWindow().GetNativeHandle());
    _swapchain = _device->CreateSwapchain(*_surface);
    _commandPool = _device->CreateCommandPool(*_commandQueue);

    _mainVsShader = _device->CreateShader("vs_shader", Shader::ShaderType::Vertex);
    _pixelShader = _device->CreateShader("ps_triangle", Shader::ShaderType::Pixel);

    opaque_shader.reset(ShaderObject::CreateShaderObject(*_mainVsShader.get(), *_pixelShader.get()));
    opaque_shader->GetVertexShader()->AddVar("model");
    opaque_shader->GetVertexShader()->AddVar("view");
    opaque_shader->GetVertexShader()->AddVar("projection");

    _buffer = _device->CreateBuffer(0, 0);
}

void Renderer::DrawMesh(const float vertices[], uint32_t vertexCount, const uint32_t indices[], uint32_t indicesCount)
{
    /*CommandBuffer& cmd = _commandPool->GetCommandBuffer();
    cmd.BeginRecording();
    cmd.BindRenderTarget(_swapchain->GetScreenTexture());


    VertexLayout layout{};
    layout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::DataType::FLOAT, true));
    layout.AddAttribute(VertexLayout::VertexAttribute(1, 2, VertexLayout::DataType::FLOAT, true));
    layout.AddAttribute(VertexLayout::VertexAttribute(2, 3, VertexLayout::DataType::FLOAT, true));*/
}

void Renderer::DrawMesh(const std::vector<float>& data, uint32_t vertex_count, Material* material, glm::mat4 mesh_pos, glm::mat4 camera, glm::mat4 projection)
{
    _buffer->BindData<float>(std::span(data.data(), data.size()));
    CommandBuffer& cmd = _commandPool->GetCommandBuffer();
    cmd.BeginRecording();
    cmd.BindRenderTarget(_swapchain->GetScreenTexture());

    VertexLayout layout{};
    layout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::DataType::FLOAT, true));
    layout.AddAttribute(VertexLayout::VertexAttribute(1, 2, VertexLayout::DataType::FLOAT, true));
    layout.AddAttribute(VertexLayout::VertexAttribute(2, 3, VertexLayout::DataType::FLOAT, true));

    cmd.BindVertexBuffer(*_buffer, layout);
    cmd.BindShaderObject(*shader_object);
    // cmd.BindTexture(texture);
    //  cmd.BindIndexBuffer( );
    //  cmd.IndexedDraw();
    shader_object->GetVertexShader()->SetMat4f("model", mesh_pos);
    shader_object->GetVertexShader()->SetMat4f("view", camera);
    shader_object->GetVertexShader()->SetMat4f("projection", projection);

    cmd.Draw(vertex_count);
    cmd.EndRecording();
    cmd.Submit();

    delete material;
}

void Renderer::DrawModel(const Model* model, glm::mat4 model_pos)
{
    const auto* meshes = model->GetMeshesPtr();

    CommandBuffer& cmd = _commandPool->GetCommandBuffer();
    cmd.BeginRecording();
    cmd.BindRenderTarget(_swapchain->GetScreenTexture());

    VertexLayout layout{};
    layout.AddAttribute(VertexLayout::VertexAttribute(0, 3, VertexLayout::DataType::FLOAT, true));
    layout.AddAttribute(VertexLayout::VertexAttribute(1, 2, VertexLayout::DataType::FLOAT, true));
    layout.AddAttribute(VertexLayout::VertexAttribute(2, 3, VertexLayout::DataType::FLOAT, true));

    current_shader_obj->GetVertexShader()->SetMat4f("model", model_pos);
    current_shader_obj->GetVertexShader()->SetMat4f("view", _camera->GetView());
    current_shader_obj->GetVertexShader()->SetMat4f("projection", _camera->GetProjection());

    _buffer->BindData<VertexData>(std::span(model->GetVerticesData(), model->GetVertexCount()));
    cmd.BindIndexBuffer(model->GetIndicesData(), model->GetIndicesCount() * sizeof(uint32_t));
    cmd.BindVertexBuffer(*_buffer, layout);

    for (const auto& mesh : *meshes)
    {
        current_shader_obj->BindMaterial(mesh->GetMaterial());
        current_shader_obj->UseMaterial();

        cmd.IndexedDraw(mesh->GetIndicesCount(), (const void*)(mesh->GetIndexStart() * sizeof(uint32_t)));
        cmd.EndRecording();
        cmd.Submit();
    }
}

void Renderer::Clear()
{
    CommandBuffer& cmd = _commandPool->GetCommandBuffer();
    cmd.Clear();
}

void Renderer::Present()
{
    _swapchain->Present();
}

void Renderer::ShowWireFrame()
{
    if (show_wireframe)
    {
        _swapchain->ShowWireFrame(true);
    }
    else
    {
        _swapchain->ShowWireFrame(false);
    }
}

void Renderer::ToggleWireFrame()
{
    show_wireframe = !show_wireframe;
}

void Renderer::ValidateWindow()
{
    _swapchain->ValidateWindow();
}

std::unique_ptr<Texture> Renderer::CreateTexture(unsigned char* buffer, int width, int height) const
{
    return _device->CreateTexture(buffer, width, height);
}

void Renderer::ChangeViewport(float x, float y, float w, float h)
{
    viewport.x = x;
    viewport.y = y;
    viewport.width = w;
    viewport.heigth = h;
    UpdateViewport();
}

void Renderer::UpdateViewport()
{
    _surface.release();
    _surface = _device->CreateSurface(Fuego::Application::Get().GetWindow().GetNativeHandle());
    _swapchain.release();
    _swapchain = _device->CreateSwapchain(*_surface);
}

VertexData::VertexData(glm::vec3 pos, glm::vec3 text_coord, glm::vec3 normal)
    : pos(pos)
    , textcoord(text_coord)
    , normal(normal)
{
}

}  // namespace Fuego::Renderer
