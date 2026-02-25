# Renderer dependencies

# cmake-lint: disable=E1126

SET(RENDERER_PUBLIC_INCLUDE_DIRECTORIES "")
SET(RENDERER_PRIVATE_INCLUDE_DIRECTORIES "")

SET(PROJECT_HEADERS
    IRenderer.hpp
    RenderViews.hpp
    # Buffer.h
    # CommandBuffer.h
    # CommandPool.h
    # CommandQueue.h
    # DescriptorBuffer.h
    # Device.h
    # Sampler.h
    Shader.h
    ShaderLibrary.h
    # Surface.h
    # Swapchain.h
    Texture.h
    Renderer.h
    Model.h
    Camera.h
    Material.h
    # ShaderObject.h
    # VertexLayout.h
    Graphics.hpp
    Image2D.h
    Color.h
    # Toolchain.h
    Skybox.h
    Framebuffer.h
    ModelFabric.h
)

SET(PROJECT_SOURCES
    Renderer.cpp
    Model.cpp
    Camera.cpp
    # VertexLayout.cpp
    Image2D.cpp
    Color.cpp
    ModelFabric.cpp
    Shader.cpp
    ShaderLibrary.cpp
)

# === Create Library ===
ADD_LIBRARY(Renderer STATIC ${PROJECT_HEADERS} ${PROJECT_SOURCES})

# === Link Libraries ===
TARGET_LINK_LIBRARIES(Renderer PRIVATE FleurInterface TBB::tbb)

# === Include Directories ===
TARGET_INCLUDE_DIRECTORIES(
  Renderer
  PRIVATE ${RENDERER_PRIVATE_INCLUDE_DIRECTORIES}
  PUBLIC ${RENDERER_PUBLIC_INCLUDE_DIRECTORIES}
)

# === Target Properties ===
SET_TARGET_PROPERTIES(Renderer PROPERTIES FOLDER "Engine/Renderer")

# Vulkan
TARGET_LINK_LIBRARIES(Renderer PRIVATE VulkanBackend)
TARGET_INCLUDE_DIRECTORIES(Renderer PRIVATE "Vulkan")
# OpenGL
TARGET_LINK_LIBRARIES(Renderer PRIVATE OpenGLBackend)
TARGET_INCLUDE_DIRECTORIES(Renderer PRIVATE "OpenGL")
