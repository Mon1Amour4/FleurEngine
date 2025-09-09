# === CoreLib - Independent Utility Library ===

# === Source Files ===
SET(CORELIB_HEADERS CoreLibConcepts.h Uptr.h Sptr.h singleton.hpp DefaultAllocator.h Bitmap.hpp Allocator.h)

SET(CORELIB_SOURCES CoreLib.cpp Allocator.cpp)

# === Library Target ===
ADD_LIBRARY(CoreLib STATIC ${CORELIB_SOURCES} ${CORELIB_HEADERS})

# === C++ Standard ===
TARGET_COMPILE_FEATURES(CoreLib PUBLIC cxx_std_${FL_CPP_LANG_VER})

# === Include Directories ===
TARGET_INCLUDE_DIRECTORIES(CoreLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# === Compile Options ===
IF(MSVC)
  TARGET_COMPILE_OPTIONS(CoreLib PRIVATE /W4 /WX)
ELSE()
  TARGET_COMPILE_OPTIONS(CoreLib PRIVATE -Wall -Wextra -Werror)
ENDIF()

# === Compile Definitions ===
TARGET_COMPILE_DEFINITIONS(
  CoreLib PRIVATE CORELIB_VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
                  CORELIB_VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR} CORELIB_BUILD_LIB
)

# === IDE Organization ===
SET_TARGET_PROPERTIES(
  CoreLib
  PROPERTIES FOLDER "Core" DEBUG_POSTFIX "_d"
)
