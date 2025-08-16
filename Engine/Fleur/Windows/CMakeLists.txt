# === Windows Dependencies ===
CMAKE_MINIMUM_REQUIRED(VERSION 3.24)

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

# === Output Directory ===
SET(OUTPUT_DIR "${CMAKE_BINARY_DIR}/output")
FILE(MAKE_DIRECTORY "${OUTPUT_DIR}")

# === Glad Configuration ===
SET(GLAD_BUILD_DIR "${CMAKE_BINARY_DIR}/Glad")
# cmake-lint: disable=E1126
FILE(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../External/glad" GLAD_PATH)

# === Glad Subdirectory ===
ADD_SUBDIRECTORY("${GLAD_PATH}" "${GLAD_BUILD_DIR}")

# === WindowsDep Target ===
ADD_LIBRARY(WindowsDep STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# === Target Configuration ===
TARGET_COMPILE_FEATURES(WindowsDep PUBLIC cxx_std_20)

TARGET_INCLUDE_DIRECTORIES(
  WindowsDep PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${GLAD_PATH}/include")

TARGET_LINK_LIBRARIES(
  WindowsDep
  PUBLIC Glad TBB::tbb
  PRIVATE FleurInterface
)

# === IDE Organization ===
SET_TARGET_PROPERTIES(
  WindowsDep
  PROPERTIES FOLDER "Platform/Windows"
             ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
             LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
             RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
)

SET_TARGET_PROPERTIES(
  Glad
  PROPERTIES FOLDER "External"
             ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
             LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
             RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
)
