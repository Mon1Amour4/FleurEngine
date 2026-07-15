cmake_minimum_required(VERSION 3.24)
cmake_policy(VERSION 3.24)

# === Configuration and Utilities ===
include(${CMAKE_CURRENT_SOURCE_DIR}/Configuration.cmake)

# === Project Definition ===
project(
  Fleur
  VERSION 0.1
  DESCRIPTION "Fleur Engine Sandbox Project"
  LANGUAGES C CXX
)

# === C++ Standard ===
set(CMAKE_CXX_STANDARD ${FL_CPP_LANG_VER})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# === Build Configuration ===
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Debug binaries get a "_d" suffix everywhere — no need to repeat per-target.
set(CMAKE_DEBUG_POSTFIX "_d")

# Iterator debug level — applies to every translation unit.
add_compile_definitions(_ITERATOR_DEBUG_LEVEL=${_ITERATOR_DEBUG_LEVEL})

# Parallel build configuration
if(NOT DEFINED CMAKE_BUILD_PARALLEL_LEVEL)
  include(ProcessorCount)
  ProcessorCount(N)
  if(NOT N EQUAL 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
  else()
    set(CMAKE_BUILD_PARALLEL_LEVEL 8)
  endif()
endif()
message(STATUS "Build parallel level: ${CMAKE_BUILD_PARALLEL_LEVEL}")

# === Output Directories ===
# Single source of truth for both single-config and multi-config generators.
if(CMAKE_CONFIGURATION_TYPES)
  foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${cfg} cfg_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg_upper} "${CMAKE_BINARY_DIR}/${cfg}/bin" CACHE PATH "" FORCE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${cfg_upper} "${CMAKE_BINARY_DIR}/${cfg}/lib" CACHE PATH "" FORCE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${cfg_upper} "${CMAKE_BINARY_DIR}/${cfg}/lib" CACHE PATH "" FORCE)
    set(CMAKE_PDB_OUTPUT_DIRECTORY_${cfg_upper}     "${CMAKE_BINARY_DIR}/${cfg}/bin" CACHE PATH "" FORCE)
  endforeach()
else()
  set(_cfg "${CMAKE_BUILD_TYPE}")
  if(NOT _cfg)
    set(_cfg "Debug")
  endif()
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_cfg}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_cfg}/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_cfg}/lib")
  set(CMAKE_PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/${_cfg}/bin")
endif()

# === IDE Configuration ===
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

# === Global Variables ===
set(ORGANIZATION "FLEUR")

# === Platform Definitions ===
if(FLEUR_PLATFORM STREQUAL "macos")
  add_compile_definitions(FLEUR_PLATFORM_MACOS)
elseif(FLEUR_PLATFORM STREQUAL "x64")
  add_compile_definitions(FLEUR_PLATFORM_WIN)
endif()

# === Testing Configuration ===
enable_testing()

# === Subdirectories ===
# Order matters: lower layers first.
add_subdirectory(MemoryManager)
add_subdirectory(Math)
add_subdirectory(CoreLib)
add_subdirectory(Tessera)
add_subdirectory(Engine)
add_subdirectory(Sandbox)

if(ENABLE_FLEUR_TEST)
  add_subdirectory(Tests)
endif()
