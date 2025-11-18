# === Memory Manager ===

# === Source Files ===
SET(PROJECT_HEADERS
    MemoryManager.h
    mmpch.h
    SLUBAllocator.hpp
    DebugDefenitions.hpp
    PageAllocator.hpp)

SET(PROJECT_SOURCES
    MemoryManager.cpp)

# === Create Library ===
ADD_LIBRARY(MemoryManager STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})

TARGET_PRECOMPILE_HEADERS(MemoryManager PRIVATE mmpch.h)

TARGET_COMPILE_FEATURES(MemoryManager PUBLIC cxx_std_20)

TARGET_COMPILE_OPTIONS(MemoryManager PRIVATE ${FLEUR_COPILATION_FLAGS})

TARGET_LINK_LIBRARIES(MemoryManager PRIVATE CoreLib)

TARGET_INCLUDE_DIRECTORIES(MemoryManager PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../CoreLib)
