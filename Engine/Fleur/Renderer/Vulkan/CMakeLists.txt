# Vulkan Backend

SET(PRIVATE_COMPILE_DEFINITIONS)
SET(VULKAN_PRIVATE_INCLUDE_DIRS)
SET(VULKAN_PUBLIC_INCLUDE_DIRS)

SET(PROJECT_HEADERS
    # IRenderer
    ../IRenderer.hpp
    ../Graphics.hpp
    ../RenderViews.hpp
    # Vulkan
    PrivateVulkanImpl.hpp
    Renderer_Vulkan.h
    FVkDevice.h
    FVkBuffer.h
    FVkCapabilities.h
    FVkCmdBuffer.h
    VkHelper.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../External/glm
)

SET(PROJECT_SOURCES
    # Vulkan
    Renderer_Vulkan.cpp
    FVkDevice.cpp
    FVkCapabilities.cpp
    FVkBuffer.cpp
    FVkCmdBuffer.cpp
)

LIST(APPEND VULKAN_PRIVATE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../)
LIST(APPEND VULKAN_PUBLIC_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../../../External/glm)

# === Create Library ===
ADD_LIBRARY(VulkanBackend STATIC ${PROJECT_HEADERS} ${PROJECT_SOURCES})

TARGET_COMPILE_DEFINITIONS(VulkanBackend
    PRIVATE
        $<$<CONFIG:Debug>:FL_CONF_DEBUG>
)

TARGET_COMPILE_DEFINITIONS(VulkanBackend PRIVATE ${PRIVATE_COMPILE_DEFINITIONS})

# Vulkan
FIND_PACKAGE(Vulkan REQUIRED)
TARGET_LINK_LIBRARIES(VulkanBackend PRIVATE Vulkan::Vulkan)
# VulkanMemoryAllocator
SET(VULKAN_MEMORY_ALLOCATOR_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../External/VulkanMemoryAllocator)
ADD_SUBDIRECTORY(${VULKAN_MEMORY_ALLOCATOR_PATH} "${CMAKE_BINARY_DIR}/VulkanMemoryAllocator")
TARGET_LINK_LIBRARIES(VulkanBackend PRIVATE GPUOpen::VulkanMemoryAllocator)
LIST(APPEND VULKAN_PRIVATE_INCLUDE_DIRS ${VULKAN_MEMORY_ALLOCATOR_PATH}/include)


TARGET_INCLUDE_DIRECTORIES(VulkanBackend PRIVATE ${VULKAN_PRIVATE_INCLUDE_DIRS})
TARGET_INCLUDE_DIRECTORIES(VulkanBackend PUBLIC ${VULKAN_PUBLIC_INCLUDE_DIRS})

# === Target Properties ===
SET_TARGET_PROPERTIES(VulkanBackend PROPERTIES FOLDER "Engine/Renderer/Backend")