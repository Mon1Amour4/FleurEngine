# Renderer dependencies

# cmake-lint: disable=E1126

SET(PUBLIC_INCLUDE_DIRECTORIES "")
SET(PRIVATE_INCLUDE_DIRECTORIES "")
SET(PUBLIC_LINK_LIBRARIES "")
SET(PRIVATE_LINK_LIBRARIES "FuegoInterface")

SET(PROJECT_HEADERS
    Buffer.h
    CommandBuffer.h
    CommandPool.h
    CommandQueue.h
    DescriptorBuffer.h
    Device.h
    Sampler.h
    Shader.h
    Surface.h
    Swapchain.h
    Texture.h
    Renderer.h
    Model.h
    Camera.h
    Material.h
    ShaderObject.h
    VertexLayout.h
    Graphics.hpp
    Image2D.h
    Color.h
    Toolchain.h
    Cubemap.h
    Skybox.h
)

SET(PROJECT_SOURCES
    Renderer.cpp
    Model.cpp
    Camera.cpp
    VertexLayout.cpp
    Image2D.cpp
    Color.cpp
    Toolchain.cpp
    Cubemap.cpp
    Skybox.cpp
)

IF(FUEGO_PLATFORM_WIN)
  LIST(APPEND PRIVATE_LINK_LIBRARIES "WindowsDep")
  LIST(APPEND PUBLIC_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../Windows")
ELSEIF(FUEGO_PLATFORM_MACOS)
  LIST(APPEND PRIVATE_LINK_LIBRARIES "MacOSDep")
  LIST(APPEND PUBLIC_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../MacOS")
ENDIF()

ADD_LIBRARY(Renderer STATIC ${PROJECT_HEADERS} ${PROJECT_SOURCES})

SET_TARGET_PROPERTIES(
  Renderer
  PROPERTIES FOLDER "Engine/Renderer"
             ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
             LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
             RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
)

TARGET_LINK_LIBRARIES(
  Renderer
  PRIVATE ${PRIVATE_LINK_LIBRARIES}
  PUBLIC ${PUBLIC_LINK_LIBRARIES}
)

TARGET_INCLUDE_DIRECTORIES(
  Renderer
  PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES}
  PUBLIC ${PUBLIC_INCLUDE_DIRECTORIES}
)

# Set properties only on existing targets
