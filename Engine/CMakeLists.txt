# === Fleur Engine ===

# === External Dependencies ===
include(${CMAKE_CURRENT_SOURCE_DIR}/ImGui.cmake)

# === Source Files ===
set(ENGINE_HEADERS
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
    # Concepts
    Fleur/Concepts.hpp
    Fleur/Console.h
    # Primitives
    Fleur/WindowPrimitives.hpp
    Fleur/AssetTypes.hpp
    Fleur/AssetHandle.h
    Fleur/AssetCache.h
    # Scene
    Fleur/Scene/Scene.h
)

set(ENGINE_SOURCES
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
    Fleur/FlTime.cpp
    Fleur/AssetsManager.cpp
    # Third Party
    External/stb_image/stb_image.cpp
    # Implementation Files
    Fleur/Console.cpp
    # Scene
    Fleur/Scene/Scene.cpp
    #Light
    Fleur/DirectionalLight.h
    Fleur/DirectionalLight.cpp
    Fleur/OmniLight.h
    Fleur/OmniLight.cpp
)

# === Engine Configuration ===
set(ENGINE_PUBLIC_DEFINITIONS FLEUR_ENABLE_ASSERTS FLEUR_BUILD_LIB)

if(FLEUR_LIB_TYPE STREQUAL "SHARED")
  list(APPEND ENGINE_PUBLIC_DEFINITIONS FLEUR_DLL_LIB)
endif()

# === Compiler Options (non-MSVC) ===
if(NOT MSVC)
  set(ENGINE_PRIVATE_COMPILE_OPTIONS -Wall -Wextra -Werror)
endif()

# === Platform Dependencies ===
set(ENGINE_DEPENDENCIES ImGui)

if(WIN32)
  list(APPEND ENGINE_DEPENDENCIES Platform)
  set(PLATFORM_SUBDIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Fleur/Platform/Windows)
elseif(APPLE)
  list(APPEND ENGINE_DEPENDENCIES MacOSDep)
  set(PLATFORM_SUBDIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Fleur/MacOS)
endif()

# === Include Directories ===
# CoreLib/MemoryManager paths arrive transitively via target_link_libraries — no manual paths needed.
set(ENGINE_PUBLIC_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Fleur
    ${CMAKE_CURRENT_SOURCE_DIR}/External/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/External/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/External/spdlog/include
)

# === FleurInterface — Shared Configuration ===
add_library(FleurInterface INTERFACE)

# Tessera is exposed through Application.h, which the lower-level Platform/Renderer
# libs compile against via FleurInterface — so its include must live here (linking
# Tessera to FleurEngine alone wouldn't reach them). The link propagates Tessera's
# PUBLIC include dir to everyone using FleurInterface.
target_link_libraries(FleurInterface INTERFACE Tessera)

target_include_directories(FleurInterface INTERFACE ${ENGINE_PUBLIC_INCLUDES})

target_precompile_headers(FleurInterface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/flpch.h)

target_compile_features(FleurInterface INTERFACE cxx_std_${FL_CPP_LANG_VER})

target_compile_definitions(FleurInterface INTERFACE ${ENGINE_PUBLIC_DEFINITIONS})

target_compile_options(FleurInterface INTERFACE ${FLEUR_COMPILATION_FLAGS})

# === Subdirectories ===
add_subdirectory(${PLATFORM_SUBDIRECTORY})
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Fleur/Lux)

# All backends are compiled; the active one is chosen at runtime.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Fleur/Lux/OpenGL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Fleur/Lux/Vulkan)

# === FleurEngine Target ===
add_library(FleurEngine ${FLEUR_LIB_TYPE} ${ENGINE_SOURCES} ${ENGINE_HEADERS})

target_link_libraries(
  FleurEngine
  PUBLIC FleurInterface ${ENGINE_DEPENDENCIES} CoreLib
  PRIVATE Lux
)
# Tessera arrives via FleurInterface (see above).
# CoreLib already brings MemoryManager transitively (PUBLIC dep).

if(NOT MSVC AND ENGINE_PRIVATE_COMPILE_OPTIONS)
  target_compile_options(FleurEngine PRIVATE ${ENGINE_PRIVATE_COMPILE_OPTIONS})
endif()

target_compile_features(FleurEngine PUBLIC cxx_std_${FL_CPP_LANG_VER})

# === IDE Organization ===
set_target_properties(FleurEngine PROPERTIES FOLDER "Engine")
set_target_properties(ImGui PROPERTIES FOLDER "External")

if(WIN32)
  target_link_libraries(FleurEngine PUBLIC Platform)
  target_include_directories(FleurEngine PUBLIC Platform)
endif()
