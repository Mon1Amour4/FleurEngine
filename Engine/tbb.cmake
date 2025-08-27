function(fix_target_output target)
    foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER ${CONFIG} CONFIG_UPPER)
        set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/${CONFIG}/bin"
            LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/${CONFIG}/lib"
            ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/${CONFIG}/lib"
            PDB_OUTPUT_DIRECTORY_${CONFIG_UPPER}     "${CMAKE_BINARY_DIR}/${CONFIG}/bin"
        )
    endforeach()
endfunction()

set(TBBMALLOC_BUILD OFF CACHE BOOL "" FORCE)
set(TBBMALLOC_PROXY_BUILD OFF CACHE BOOL "" FORCE)

set(BUILD_SHARED_LIBS ON CACHE BOOL "Force shared build" FORCE)
option(TBB_BUILD_SHARED "Build TBB as shared library" ON)
option(TBB_BUILD_STATIC "Build TBB as static library" OFF)
option(TBB_BUILD_TBBMALLOC "Build TBB scalable memory allocator" OFF)
option(TBB_BUILD_TBBMALLOC_PROXY "Build malloc replacement library" OFF)
option(TBB_BUILD_TESTS "Build internal TBB tests (not recommended)" OFF)
option(TBB_TEST "Enable testing " OFF)
option(TBB_STRICT "Treat compiler warnings as errors " ON)
option(TBB_ENABLE_RUNTIME_DEPENDENCY_VERIFICATION "Enable runtime dependency signature verification" ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/External/tbb)

SET_TARGET_PROPERTIES(
  tbb
  PROPERTIES FOLDER "External"
)
fix_target_output(tbb)
