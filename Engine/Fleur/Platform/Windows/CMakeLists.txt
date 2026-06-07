# === Windows Platform Dependencies ===

set(PROJECT_HEADERS
    WindowWin.h
    EventQueueWin.h
    InputWin.h
    KeyCodesWin.h
    TimeWin.h
)

set(PROJECT_SOURCES
    EventQueueWin.cpp
    InputWin.cpp
    WindowWin.cpp
    EntryPointWindows.cpp
    TimeWin.cpp
)

# === Create Library ===
add_library(Platform STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})

target_compile_features(Platform PUBLIC cxx_std_${FL_CPP_LANG_VER})

# === Link Libraries ===
# CoreLib pulls in singleton.hpp and (transitively) MemoryManager headers used by Application.h.
# opengl32 is needed regardless of render backend — WindowWin.cpp uses WGL extensions.
target_link_libraries(Platform PRIVATE FleurInterface CoreLib opengl32)

# === Include Directories ===
target_include_directories(Platform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# === Target Properties ===
set_target_properties(Platform PROPERTIES FOLDER "Platform/Windows")
