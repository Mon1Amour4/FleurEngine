# --- Fleur Global Configuration ---
# Defines project-wide variables, helpers, and compile defaults.
# Included once from the root CMakeLists.txt before PROJECT().

# C++ language standard version
set(FL_CPP_LANG_VER "20" CACHE STRING "C++ Language standard version")

# Iterator Debug Level
set(_ITERATOR_DEBUG_LEVEL "0" CACHE STRING "Iterator Debug Level (0=disabled, 1=basic, 2=extended)")

# MSVC runtime: single generator expression handles both Debug and Release
# in single-config (Ninja) and multi-config (VS) generators.
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

  # Keep MSVC's C++26 contiguous-container annotations consistent across
  # Fleur targets and third-party static libraries such as GoogleTest.
  # Without this, gtest.lib and FleurTests.obj can emit different
  # detect_mismatch values for string/vector annotations.
  add_compile_definitions(_DISABLE_STRING_ANNOTATION _DISABLE_VECTOR_ANNOTATION)
endif()

# Platform detection
if(APPLE)
  set(FLEUR_PLATFORM "macos" CACHE STRING "Current platform")
elseif(WIN32)
  set(FLEUR_PLATFORM "x64" CACHE STRING "Current platform")
endif()

# Testing toggle (consumed by root)
option(ENABLE_FLEUR_TEST "Enable Fleur testing" OFF)

# Library type for the FleurEngine target (STATIC or SHARED)
set(FLEUR_LIB_TYPE "STATIC" CACHE STRING "Library type (STATIC/SHARED)")

# Render backend selection — controls which backends are built.
# OpenGL files are preserved on disk; toggle to re-enable them.
set(FLEUR_RENDER_BACKEND "Vulkan" CACHE STRING "Render backend: Vulkan, OpenGL, or Both")
set_property(CACHE FLEUR_RENDER_BACKEND PROPERTY STRINGS "Vulkan" "OpenGL" "Both")

# Project-wide compiler flags (MSVC). Consumed via FleurInterface and per-module options.
set(FLEUR_COMPILATION_FLAGS
    /permissive- /Zc:checkGwOdr /Zc:enumTypes /Zc:inline /Zc:templateScope
    /GR- /EHsc /volatile:iso /utf-8 /W4
    # /WX
)

# --- Helpers ---

# Apply standard Fleur output directories to a target.
# Works for both single-config (Ninja, Make) and multi-config (VS, Xcode) generators.
function(fleur_set_output_dirs target)
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib"
    PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin"
  )
  foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${cfg} cfg_upper)
    set_target_properties(${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY_${cfg_upper} "${CMAKE_BINARY_DIR}/${cfg}/bin"
      LIBRARY_OUTPUT_DIRECTORY_${cfg_upper} "${CMAKE_BINARY_DIR}/${cfg}/lib"
      ARCHIVE_OUTPUT_DIRECTORY_${cfg_upper} "${CMAKE_BINARY_DIR}/${cfg}/lib"
      PDB_OUTPUT_DIRECTORY_${cfg_upper}     "${CMAKE_BINARY_DIR}/${cfg}/bin"
    )
  endforeach()
endfunction()
