# === Fleur Engine ===

# === External Dependencies ===
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/ImGui.cmake)
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/tbb.cmake)

# === Source Files ===
SET(ENGINE_HEADERS
    # Core Engine
    Fleur/Application.h
    Fleur/Core.h
    Fleur/EntryPoint.h
    Fleur/Log.h
    Fleur/Window.h
    Fleur/EventQueue.h
    # Event System
    Fleur/Events/Event.h
    Fleur/Events/EventVisitor.h
    Fleur/Events/KeyEvent.h
    Fleur/Events/MouseEvent.h
    Fleur/Events/ApplicationEvent.h
    # Input System
    Fleur/MouseCodes.h
    Fleur/KeyCodes.h
    Fleur/Input.h
    # Layer System
    Fleur/Layer.h
    Fleur/LayerStack.h
    Fleur/ImGui/ImGuiLayer.h
    # Renderer
    Fleur/Renderer
    # File System
    Fleur/FileSystem/FileSystem.h
    # Service System
    Fleur/Services/ServiceInterfaces.hpp
    Fleur/Services/ServiceLocator.h
    # Utilities
    Fleur/ThreadPool.h
    Fleur/FlTime.h
    Fleur/AssetsManager.h
    # Third Party
    External/stb_image/stb_image.h
    External/stb_image/stb_image_write.h
    # Main Headers
    Fleur.h
    flpch.h
)

SET(ENGINE_SOURCES
    # Core Engine
    Fleur/Application.cpp
    Fleur/Log.cpp
    # Layer System
    Fleur/Layer.cpp
    Fleur/LayerStack.cpp
    Fleur/ImGui/ImGuiLayer.cpp
    # File System
    Fleur/FileSystem/FileSystem.cpp
    # Service System
    Fleur/Services/ServiceLocator.cpp
    # Utilities
    Fleur/ThreadPool.cpp
    Fleur/AssetsManager.cpp
    # Third Party
    External/stb_image/stb_image.cpp
    # Implementation Files
)

# === Engine Configuration ===
SET(ENGINE_PUBLIC_DEFINITIONS FLEUR_ENABLE_ASSERTS FLEUR_BUILD_LIB)

# Ensure _ITERATOR_DEBUG_LEVEL is set for all targets
SET(ENGINE_PRIVATE_DEFINITIONS _ITERATOR_DEBUG_LEVEL=${_ITERATOR_DEBUG_LEVEL})

IF(FLEUR_LIB_TYPE STREQUAL "SHARED")
  LIST(APPEND ENGINE_PUBLIC_DEFINITIONS FLEUR_DLL_LIB)
ENDIF()

# === Compiler Options ===
IF(MSVC)
  SET(ENGINE_PRIVATE_COMPILE_OPTIONS /W4 /permissive-)
  SET(ENGINE_PUBLIC_COMPILE_OPTIONS /utf-8)
ELSE()
  SET(ENGINE_PRIVATE_COMPILE_OPTIONS -Wall -Wextra -Werror)
  SET(ENGINE_PUBLIC_COMPILE_OPTIONS "")
ENDIF()

# === Platform Dependencies ===
SET(ENGINE_DEPENDENCIES ImGui TBB::tbb)

IF(FLEUR_PLATFORM STREQUAL "win")
  LIST(APPEND ENGINE_DEPENDENCIES WindowsDep)
  SET(PLATFORM_SUBDIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Fleur/Windows)
ELSEIF(FLEUR_PLATFORM STREQUAL "macos")
  LIST(APPEND ENGINE_DEPENDENCIES MacOSDep)
  SET(PLATFORM_SUBDIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Fleur/MacOS)
ENDIF()

# === Include Directories ===
SET(ENGINE_PUBLIC_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Fleur
    ${CMAKE_CURRENT_SOURCE_DIR}/Fleur/Renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/External/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/External/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/External/spdlog/include
    ${CMAKE_SOURCE_DIR}/CoreLib
)

# === FleurInterface - Shared Configuration ===
ADD_LIBRARY(FleurInterface INTERFACE)

TARGET_INCLUDE_DIRECTORIES(FleurInterface INTERFACE ${ENGINE_PUBLIC_INCLUDES})

TARGET_PRECOMPILE_HEADERS(FleurInterface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/flpch.h)

TARGET_COMPILE_FEATURES(FleurInterface INTERFACE cxx_std_20)

TARGET_COMPILE_DEFINITIONS(
  FleurInterface INTERFACE ${ENGINE_PUBLIC_DEFINITIONS} _ITERATOR_DEBUG_LEVEL=${_ITERATOR_DEBUG_LEVEL}
)

TARGET_COMPILE_OPTIONS(FleurInterface INTERFACE ${ENGINE_PUBLIC_COMPILE_OPTIONS} ${ENGINE_PRIVATE_COMPILE_OPTIONS})

# === Subdirectories ===
ADD_SUBDIRECTORY(${PLATFORM_SUBDIRECTORY})
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Fleur/Renderer)

# === FleurEngine Target ===
ADD_LIBRARY(FleurEngine ${FLEUR_LIB_TYPE} ${ENGINE_SOURCES} ${ENGINE_HEADERS})

TARGET_LINK_LIBRARIES(
  FleurEngine
  PUBLIC FleurInterface ${ENGINE_DEPENDENCIES} CoreLib
  PRIVATE Renderer
)

TARGET_INCLUDE_DIRECTORIES(FleurEngine PRIVATE)

TARGET_COMPILE_DEFINITIONS(FleurEngine PRIVATE ${ENGINE_PRIVATE_DEFINITIONS})

TARGET_COMPILE_FEATURES(FleurEngine PUBLIC cxx_std_20)

# === IDE Organization ===
SET_TARGET_PROPERTIES(
  FleurEngine
  PROPERTIES FOLDER "Engine" DEBUG_POSTFIX "_d"
)

SET_TARGET_PROPERTIES(
  ImGui
  PROPERTIES FOLDER "External"
)