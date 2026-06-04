# TBB integration. Output dirs unified via fleur_set_output_dirs (Configuration.cmake).

set(TBBMALLOC_BUILD OFF CACHE BOOL "" FORCE)
set(TBBMALLOC_PROXY_BUILD OFF CACHE BOOL "" FORCE)

set(BUILD_SHARED_LIBS ON CACHE BOOL "Force shared build" FORCE)
option(TBB_BUILD_SHARED "Build TBB as shared library" ON)
option(TBB_BUILD_STATIC "Build TBB as static library" OFF)
option(TBB_BUILD_TBBMALLOC "Build TBB scalable memory allocator" OFF)
option(TBB_BUILD_TBBMALLOC_PROXY "Build malloc replacement library" OFF)
option(TBB_BUILD_TESTS "Build internal TBB tests (not recommended)" OFF)
option(TBB_TEST "Enable testing " OFF)
option(TBB_STRICT "Treat compiler warnings as errors " OFF)
option(TBB_ENABLE_RUNTIME_DEPENDENCY_VERIFICATION "Enable runtime dependency signature verification" ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/External/tbb)

set_target_properties(tbb PROPERTIES FOLDER "External")
fleur_set_output_dirs(tbb)
