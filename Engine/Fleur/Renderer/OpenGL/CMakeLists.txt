# OpenGL Backend

SET(PROJECT_HEADERS
    ../IRenderer.hpp
    ../Graphics.hpp
    ../RenderViews.hpp
    Renderer_OpenGL.h
    # BufferOpenGL.h
    # CommandBufferOpenGL.h
    # CommandPoolOpenGl.h
    # CommandQueueOpenGL.h
    # DeviceOpenGL.h
    # FramebufferOpenGL.h
    # ShaderObjectOpenGL.h
    # ShaderOpenGL.h
    # SurfaceOpenGL.h
    # SwapchainOpenGL.h
    # TextureOpenGL.h
)

SET(PROJECT_SOURCES
    Renderer_OpenGL.cpp
    # BufferOpenGL.cpp
    # CommandBufferOpenGL.cpp
    # CommandPoolOpenGl.cpp
    # CommandQueueOpenGL.cpp
    # DeviceOpenGL.cpp
    # FramebufferOpenGL.cpp
    # MaterialOpenGL.cpp
    # ShaderObjectOpenGL.cpp
    # ShaderOpenGL.cpp
    # SurfaceOpenGL.cpp
    # SwapchainOpenGL.cpp
    # TextureOpenGL.cpp
)


# === Create Library ===
ADD_LIBRARY(OpenGLBackend STATIC ${PROJECT_HEADERS} ${PROJECT_SOURCES})

TARGET_LINK_LIBRARIES(
  OpenGLBackend  
  PRIVATE FleurInterface TBB::tbb CoreLib 
)

# === Target Properties ===
SET_TARGET_PROPERTIES(OpenGLBackend PROPERTIES FOLDER "Engine/Renderer/Backend")

# === Glad Configuration ===
SET(GLAD_BUILD_DIR "${CMAKE_BINARY_DIR}/Glad")
FILE(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../External/glad" GLAD_SOURCES)

# === Glad Subdirectory ===
ADD_SUBDIRECTORY("${GLAD_SOURCES}" "${GLAD_BUILD_DIR}")

TARGET_LINK_LIBRARIES(OpenGLBackend PRIVATE Glad)

SET_TARGET_PROPERTIES(Glad PROPERTIES FOLDER "External")
