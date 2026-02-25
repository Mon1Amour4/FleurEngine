# === Windows Dependencies ===

# === Source Files ===
SET(PROJECT_HEADERS
    WindowLinux.h
    ../Windows/EventQueueWin.h
    InputLinux.h
    ../Windows/TimeWin.h
)

SET(PROJECT_SOURCES
    ../Windows/EventQueueWin.cpp
    InputLinux.cpp
    WindowLinux.cpp
    EntryPointLinux.cpp
    ../Windows/TimeWin.cpp
)

# cmake-lint: disable=E1126

# === Create Library ===
ADD_LIBRARY(Platform STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# === WindowsDep Configuration ===
TARGET_COMPILE_FEATURES(Platform PUBLIC cxx_std_20)

# === Link Libraries ===
TARGET_LINK_LIBRARIES(
  Platform
  PRIVATE FleurInterface wayland-client wayland-egl
)

# === Include Directories ===
TARGET_INCLUDE_DIRECTORIES(
  Platform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# === Target Properties ===
SET_TARGET_PROPERTIES(Platform PROPERTIES FOLDER "Platform/Linux")
