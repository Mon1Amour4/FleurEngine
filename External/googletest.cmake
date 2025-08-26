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

SET_TARGET_PROPERTIES(
  gtest gtest_main
  PROPERTIES FOLDER "Tests"
  PDB_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/Debug/bin"
  PDB_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/Release/bin"
  PDB_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo/bin"
  PDB_OUTPUT_DIRECTORY_MINSIZEREL      "${CMAKE_BINARY_DIR}/MinSizeRel/bin"
)
get_target_property(_gtest_pdb_dir gtest PDB_OUTPUT_DIRECTORY_DEBUG)
message("gtest PDB_DEBUG = ${_gtest_pdb_dir}")

get_target_property(_gtestmain_pdb_dir gtest_main PDB_OUTPUT_DIRECTORY_DEBUG)
message("gtest_main PDB_DEBUG = ${_gtestmain_pdb_dir}")
