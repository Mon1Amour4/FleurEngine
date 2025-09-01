set(GOOGLETEST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../External/googletest")
set(GOOGLETEST_BINATY_DIR ${CMAKE_BINARY_DIR}/External/googletest)

set(CMAKE_DEBUG_POSTFIX "")

#get_target_property(pdb_debug_postfix ${name} DEBUG_POSTFIX) 
# For Windows: Prevent overriding the parent project's compiler/linker settings "Use shared (DLL) run-time lib even when
# Google Test is built as static lib."
ENABLE_TESTING()
IF(FLEUR_PLATFORM_WIN)
  SET(GTEST_FORCE_SHARED_CRT
      ON
      CACHE BOOL "" FORCE
  )
ENDIF()
SET(BUILD_GMOCK
    OFF
    CACHE BOOL "" FORCE
)
SET(INSTALL_GTEST
    OFF
    CACHE BOOL "" FORCE
)
SET(GTEST_HAS_ABSL
    OFF
    CACHE BOOL "" FORCE
)
SET(GTEST_SOLUTION ${FLEUR_ROOT}/build/${FLEUR_PLATFORM}/External)
# End googletest

add_subdirectory(${GOOGLETEST_ROOT} ${GOOGLETEST_BINATY_DIR})

# Need to set this PDB outputs cause googletest build as STATIC lib, and for static libs MSVC has two separate paths
# So pdb files creates at Config/bin (as user defined output for all pdb files) AND lib/Config (as for all STATIC libs)
SET_TARGET_PROPERTIES(
  gtest gtest_main
  PROPERTIES FOLDER "Tests"
        COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/Debug/bin"
        COMPILE_PDB_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/Release/bin"
        COMPILE_PDB_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo/bin"
        COMPILE_PDB_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}/MinSizeRel/bin"
)
