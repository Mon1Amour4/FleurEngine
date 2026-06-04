# === CoreLib — Utility Layer (allocator adapters, smart pointers, concepts) ===
#
# Layering: CoreLib sits ABOVE MemoryManager. FleurAllocator.hpp wraps MM types,
# so MemoryManager is a PUBLIC dependency (its headers leak through ours).

set(CORELIB_HEADERS
    CoreLibConcepts.h
    Uptr.h
    Sptr.h
    singleton.hpp
    DefaultAllocator.h
    Bitmap.hpp
    FleurAllocator.hpp
)

set(CORELIB_SOURCES
    CoreLib.cpp
)
# DefaultAllocator.cpp omitted intentionally — depends on Engine/Core.h
# (legacy layering issue, to be sorted out when DefaultAllocator is revived).

add_library(CoreLib STATIC ${CORELIB_SOURCES} ${CORELIB_HEADERS})

target_compile_features(CoreLib PUBLIC cxx_std_${FL_CPP_LANG_VER})

target_include_directories(CoreLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
  target_compile_options(CoreLib PRIVATE /W4 /WX /utf-8)
else()
  target_compile_options(CoreLib PRIVATE -Wall -Wextra -Werror)
endif()

target_compile_definitions(CoreLib
  PRIVATE
    CORELIB_VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
    CORELIB_VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
    CORELIB_BUILD_LIB
)

# PUBLIC: FleurAllocator.hpp publishes MM:: types.
target_link_libraries(CoreLib PUBLIC MemoryManager)

set_target_properties(CoreLib PROPERTIES FOLDER "Core")
