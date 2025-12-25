# OpenGL Backend

SET(PROJECT_HEADERS
    ../IRenderer.hpp
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

# === Target Properties ===
SET_TARGET_PROPERTIES(OpenGLBackend PROPERTIES FOLDER "Engine/Renderer/Backend")

TARGET_LINK_LIBRARIES(
  OpenGLBackend  
  PRIVATE FleurInterface Glad TBB::tbb CoreLib
)