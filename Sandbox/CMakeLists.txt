# === Sandbox Application ===

# === Source Files ===
SET(SANDBOX_HEADERS SandboxApp.h)

SET(SANDBOX_SOURCES SandboxApp.cpp)

# === Platform-specific Configuration ===
IF(FLEUR_PLATFORM STREQUAL "macos")
  # macOS Sources
  LIST(APPEND SANDBOX_SOURCES MacOS/main.mm)

  # macOS Resources
  SET(MACOS_RESOURCES Resources/MacOS/Assets.xcassets Resources/MacOS/Base.lproj/Main.storyboard
                      Resources/MacOS/Triangle.metal
  )

  # Create macOS Bundle
  ADD_EXECUTABLE(Sandbox MACOSX_BUNDLE ${SANDBOX_SOURCES} ${SANDBOX_HEADERS} ${MACOS_RESOURCES})

  # Bundle Configuration
  SET_SOURCE_FILES_PROPERTIES(${MACOS_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  SET_TARGET_PROPERTIES(
    Sandbox
    PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${ORGANIZATION}.Sandbox"
               MACOSX_BUNDLE_GUI_IDENTIFIER "${ORGANIZATION}.Sandbox"
               MACOSX_BUNDLE_BUNDLE_NAME "${ORGANIZATION}.Sandbox"
               MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Resources/MacOS/Sandbox-Info.plist.in"
  )

ELSEIF(FLEUR_PLATFORM STREQUAL "x64")
  # Windows Sources
  # DebugVisualizers.cpp: func-eval targets for natvis (mat4 scale/euler, color hex). MSVC-only.
  LIST(APPEND SANDBOX_SOURCES Windows/main.cpp DebugVisualizers.cpp)

  # Create Windows Executable
  ADD_EXECUTABLE(Sandbox WIN32 ${SANDBOX_SOURCES} ${SANDBOX_HEADERS})

  # Windows-specific properties
  SET_TARGET_PROPERTIES(Sandbox PROPERTIES WIN32_EXECUTABLE TRUE)
ENDIF()

# === Debugger Visualizers (MSVC / VS natvis) ===
# Embed natvis into the Sandbox PDB so the Visual Studio / VS Code (cppvsdbg)
# debugger renders glm types (vec, mat4, quat) readably instead of raw arrays.
# PDB-embedded -> no per-machine Visualizers-folder setup, survives clean checkout.
IF(MSVC)
  TARGET_SOURCES(Sandbox PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/External/glm/util/glm.natvis
    ${CMAKE_SOURCE_DIR}/Engine/Fleur/Fleur.natvis
  )
ENDIF()

# === Dependencies ===
TARGET_LINK_LIBRARIES(Sandbox PRIVATE FleurEngine)

TARGET_COMPILE_FEATURES(Sandbox PRIVATE cxx_std_20)

# === IDE Organization ===
set_target_properties(Sandbox PROPERTIES FOLDER "Applications")
