# === Lux ===
#
# Files commented out below (Buffer.h, CommandBuffer.h, Surface.h, Swapchain.h, ...)
# are the abstract-interface headers from the OpenGL era. They live on disk and
# remain #include-able from Lux.h; they are intentionally not advertised
# in the IDE filters until the OpenGL backend is reactivated.

set(PROJECT_HEADERS
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
    # Surface.h
    # Swapchain.h
    Texture.h
    Lux.h
    Model.h
    Camera.h
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

set(PROJECT_SOURCES
    Lux.cpp
    Model.cpp
    Camera.cpp
    # VertexLayout.cpp
    Image2D.cpp
    Color.cpp
    ModelFabric.cpp
    Shader.cpp
)

# === Create Library ===
add_library(Lux STATIC ${PROJECT_HEADERS} ${PROJECT_SOURCES})

# === Link Libraries ===
# CoreLib is linked because Lux.cpp / Image2D.h pull FleurAllocator.hpp / Bitmap.hpp.
target_link_libraries(Lux PRIVATE FleurInterface CoreLib)

# === Target Properties ===
set_target_properties(Lux PROPERTIES FOLDER "Engine/Lux")

# === Backend Selection (driven by FLEUR_RENDER_BACKEND) ===
# All backends compiled; runtime selection (no compile-time backend define).
target_link_libraries(Lux PRIVATE VulkanBackend OpenGLBackend)
target_include_directories(Lux PRIVATE "Vulkan" "OpenGL")
