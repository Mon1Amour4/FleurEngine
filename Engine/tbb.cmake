set(_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/build/${FUEGO_PLATFORM}/output)

foreach(OUTPUTCONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${_OUTPUT_DIR}/${OUTPUTCONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${_OUTPUT_DIR}/${OUTPUTCONFIG})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${_OUTPUT_DIR}/${OUTPUTCONFIG})
endforeach()

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

function(set_output_dirs target)
    if(TARGET ${target})
        foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
            string(TOUPPER ${config} CONFIG_UP)
            set_target_properties(${target} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UP} ${CMAKE_SOURCE_DIR}/build/${FUEGO_PLATFORM}/output/${config}
                ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UP} ${CMAKE_SOURCE_DIR}/build/${FUEGO_PLATFORM}/output/${config}
                LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UP} ${CMAKE_SOURCE_DIR}/build/${FUEGO_PLATFORM}/output/${config}
                PDB_OUTPUT_DIRECTORY_${CONFIG_UP}     ${CMAKE_SOURCE_DIR}/build/${FUEGO_PLATFORM}/output/${config}
            )
        endforeach()
    endif()
endfunction()

foreach(tgt IN ITEMS tbb tbbmalloc tbbmalloc_proxy)
    set_output_dirs(${tgt})
endforeach()

SET_TARGET_PROPERTIES(
  tbb
  PROPERTIES FOLDER "External"
)