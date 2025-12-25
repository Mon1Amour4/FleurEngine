# === Windows Dependencies ===

# === Source Files ===
SET(PROJECT_HEADERS
    EventQueueWin.h
    InputWin.h
    KeyCodesWin.h
    WindowWin.h
    TimeWin.h
)

SET(PROJECT_SOURCES
    EventQueueWin.cpp
    InputWin.cpp
    WindowWin.cpp
    EntryPointWindows.cpp
    TimeWin.cpp
)

# cmake-lint: disable=E1126

# === Create Library ===
ADD_LIBRARY(Platform STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# === WindowsDep Configuration ===
TARGET_COMPILE_FEATURES(Platform PUBLIC cxx_std_20)

# === Link Libraries ===
TARGET_LINK_LIBRARIES(
  Platform  
  PRIVATE FleurInterface TBB::tbb
)

# === Include Directories ===
TARGET_INCLUDE_DIRECTORIES(
  Platform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# === Target Properties ===
SET_TARGET_PROPERTIES(Platform PROPERTIES FOLDER "Platform/Windows")
