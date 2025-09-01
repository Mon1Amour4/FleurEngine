# === Windows Dependencies ===

# === Source Files ===
SET(PROJECT_HEADERS
    EventQueueWin.h
    InputWin.h
    KeyCodesWin.h
    WindowWin.h
    FileSystemPathsWin.h
    OpenGL/BufferOpenGL.h
    OpenGL/DeviceOpenGL.h
    OpenGL/CommandBufferOpenGL.h
    OpenGL/ShaderOpenGL.h
    OpenGL/CommandQueueOpenGL.h
    OpenGL/SwapchainOpenGL.h
    OpenGL/CommandPoolOpenGL.h
    OpenGL/TextureOpenGL.h
    OpenGL/SurfaceOpenGL.h
    OpenGL/ShaderObjectOpenGL.h
    OpenGL/FramebufferOpenGL.h
    TimeWin.h
)

SET(PROJECT_SOURCES
    EventQueueWin.cpp
    InputWin.cpp
    WindowWin.cpp
    EntryPointWindows.cpp
    OpenGL/BufferOpenGL.cpp
    OpenGL/DeviceOpenGL.cpp
    OpenGL/CommandBufferOpenGL.cpp
    OpenGL/ShaderOpenGL.cpp
    OpenGL/CommandQueueOpenGL.cpp
    OpenGL/SwapchainOpenGL.cpp
    OpenGL/CommandPoolOpenGL.cpp
    OpenGL/TextureOpenGL.cpp
    OpenGL/SurfaceOpenGL.cpp
    OpenGL/ShaderObjectOpenGL.cpp
    OpenGL/FramebufferOpenGL.cpp
    TimeWin.cpp
)

# cmake-lint: disable=E1126

# === Create Library ===
ADD_LIBRARY(WindowsDep STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# === WindowsDep Configuration ===
TARGET_COMPILE_FEATURES(WindowsDep PUBLIC cxx_std_20)

# === Glad Configuration ===
SET(GLAD_BUILD_DIR "${CMAKE_BINARY_DIR}/Glad")
FILE(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../External/glad" GLAD_SOURCES)

# === Glad Subdirectory ===
ADD_SUBDIRECTORY("${GLAD_SOURCES}" "${GLAD_BUILD_DIR}")

# === Link Libraries ===
TARGET_LINK_LIBRARIES(
  WindowsDep  
  PRIVATE FleurInterface TBB::tbb Glad
)

# === Include Directories ===
TARGET_INCLUDE_DIRECTORIES(
  WindowsDep PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# === Target Properties ===
SET_TARGET_PROPERTIES(WindowsDep PROPERTIES FOLDER "Platform/Windows")
SET_TARGET_PROPERTIES(Glad PROPERTIES FOLDER "External")
