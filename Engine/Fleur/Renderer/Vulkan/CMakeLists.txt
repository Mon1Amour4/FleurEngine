# Vulkan Backend

SET(PROJECT_HEADERS
    # IRenderer
    ../IRenderer.hpp
    # Vulkan
    Renderer_Vulkan.h
    VkDevice.h
)

SET(PROJECT_SOURCES
    # Vulkan
    Renderer_Vulkan.cpp
    VkDevice.cpp
)


# === Create Library ===
ADD_LIBRARY(Renderer_Vulkan_Backend SHARED ${PROJECT_HEADERS} ${PROJECT_SOURCES})

# === Target Properties ===
SET_TARGET_PROPERTIES(Renderer_Vulkan_Backend PROPERTIES FOLDER "Engine/Renderer/Backend")

TARGET_COMPILE_DEFINITIONS(Renderer_Vulkan_Backend PRIVATE "DLL_EXPORT")

# Vulkan
FIND_PACKAGE(Vulkan)
TARGET_LINK_LIBRARIES(Renderer_Vulkan_Backend PRIVATE Vulkan::Vulkan)

TARGET_INCLUDE_DIRECTORIES(
  Renderer_Vulkan_Backend
  PRIVATE "../"
)


