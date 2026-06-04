# === MemoryManager — Foundation Layer (allocators, bitmap utilities) ===
#
# Layering: MemoryManager has no dependency on CoreLib or Engine.

set(PROJECT_HEADERS
    MemoryManager.h
    mmpch.h
    SLUBAllocator.hpp
    MemoryDefinitions.hpp
    PageAllocator.hpp
    TLSFAllocator.hpp
    BitSet64.h
)

set(PROJECT_SOURCES
    MemoryManager.cpp
    BitSet64.cpp
)

add_library(MemoryManager STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})

target_precompile_headers(MemoryManager PRIVATE mmpch.h)

target_compile_features(MemoryManager PUBLIC cxx_std_${FL_CPP_LANG_VER})

target_compile_options(MemoryManager PRIVATE ${FLEUR_COMPILATION_FLAGS})

target_compile_definitions(MemoryManager PUBLIC MEMORYMANAGER_PROFILING)

target_include_directories(MemoryManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(MemoryManager PROPERTIES FOLDER "Core")
