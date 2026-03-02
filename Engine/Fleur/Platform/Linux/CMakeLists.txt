# === Windows Dependencies ===

# === Source Files ===
SET(PROJECT_HEADERS
    WindowLinux.h
    InputLinux.h
    KeyCodesLinux.h
    EventQueueLinux.h
    ../Windows/TimeWin.h
    Wayland.h
)

SET(PROJECT_SOURCES
    InputLinux.cpp
    EventQueueLinux.cpp
    WindowLinux.cpp
    EntryPointLinux.cpp
    ../Windows/TimeWin.cpp
    Wayland.cpp
)

# cmake-lint: disable=E1126

# === Create Library ===
ADD_LIBRARY(Platform STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# === WindowsDep Configuration ===
TARGET_COMPILE_FEATURES(Platform PUBLIC cxx_std_20)

# === Link Libraries ===
TARGET_LINK_LIBRARIES(
  Platform
  PRIVATE FleurInterface wayland-client
)

# === Include Directories ===
TARGET_INCLUDE_DIRECTORIES(
  Platform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# === Target Properties ===
SET_TARGET_PROPERTIES(Platform PROPERTIES FOLDER "Platform/Linux")
