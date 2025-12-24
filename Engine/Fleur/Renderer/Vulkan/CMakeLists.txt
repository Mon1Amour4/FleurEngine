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

SET(PRIVATE_COMPILE_DEFINITIONS)

# === Create Library ===
ADD_LIBRARY(Renderer_Vulkan_Backend SHARED ${PROJECT_HEADERS} ${PROJECT_SOURCES})

# === Target Properties ===
SET_TARGET_PROPERTIES(Renderer_Vulkan_Backend PROPERTIES FOLDER "Engine/Renderer/Backend")

TARGET_COMPILE_DEFINITIONS(Renderer_Vulkan_Backend
    PRIVATE
        $<$<CONFIG:Debug>:FL_CONF_DEBUG>
)

LIST(APPEND PRIVATE_COMPILE_DEFINITIONS "DLL_EXPORT")
TARGET_COMPILE_DEFINITIONS(Renderer_Vulkan_Backend PRIVATE ${PRIVATE_COMPILE_DEFINITIONS})

# Vulkan
FIND_PACKAGE(Vulkan REQUIRED)
TARGET_LINK_LIBRARIES(Renderer_Vulkan_Backend PRIVATE Vulkan::Vulkan)

TARGET_INCLUDE_DIRECTORIES(
  Renderer_Vulkan_Backend
  PRIVATE "../"
)



