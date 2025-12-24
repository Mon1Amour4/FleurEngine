# Renderer dependencies

# cmake-lint: disable=E1126

SET(RENDERER_PUBLIC_INCLUDE_DIRECTORIES "")
SET(RENDERER_PRIVATE_INCLUDE_DIRECTORIES "")

SET(PROJECT_HEADERS
    IRenderer.hpp
    # Buffer.h
    # CommandBuffer.h
    # CommandPool.h
    # CommandQueue.h
    # DescriptorBuffer.h
    # Device.h
    # Sampler.h
    # Shader.h
    # Surface.h
    # Swapchain.h
    Texture.h
    Renderer.h
    Model.h
    Camera.h
    # Material.h
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
)

# === Create Library ===
ADD_LIBRARY(Renderer STATIC ${PROJECT_HEADERS} ${PROJECT_SOURCES})

# === Link Libraries ===
TARGET_LINK_LIBRARIES(Renderer PRIVATE FleurInterface TBB::tbb)

IF(FLEUR_PLATFORM_WIN)
  FILE(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../Windows" REAL_WINDOWS_PATH)
  LIST(APPEND RENDERER_PUBLIC_INCLUDE_DIRECTORIES ${REAL_WINDOWS_PATH})
  TARGET_LINK_LIBRARIES(Renderer PUBLIC WindowsDep)
ELSEIF(FLEUR_PLATFORM_MACOS)
  TARGET_LINK_LIBRARIES(Renderer PUBLIC MacOSDep)
  FILE(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../MacOS" REAL_MACOS_PATH)
  LIST(APPEND RENDERER_PUBLIC_INCLUDE_DIRECTORIES "${REAL_MACOS_PATH}")
ENDIF()

# === Include Directories ===
TARGET_INCLUDE_DIRECTORIES(
  Renderer
  PRIVATE ${RENDERER_PRIVATE_INCLUDE_DIRECTORIES}
  PUBLIC ${RENDERER_PUBLIC_INCLUDE_DIRECTORIES}
)

# === Target Properties ===
SET_TARGET_PROPERTIES(Renderer PROPERTIES FOLDER "Engine/Renderer")
