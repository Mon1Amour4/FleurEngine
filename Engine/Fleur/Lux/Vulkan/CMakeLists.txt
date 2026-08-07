# Vulkan Backend

SET(PRIVATE_COMPILE_DEFINITIONS)
SET(VULKAN_PRIVATE_INCLUDE_DIRS)
SET(VULKAN_PUBLIC_INCLUDE_DIRS)
SET(VULKAN_SPIRV_REFLECT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../External/SPIRV-Reflect)

#ADD_SUBDIRECTORY(${VULKAN_SPIRV_REFLECT_PATH})

SET(PROJECT_HEADERS
    # IRenderer
    ../IRenderer.hpp
    ../Graphics.hpp
    ../RenderViews.hpp
    # Vulkan
    PrivateVulkanImpl.hpp
    Renderer_Vulkan.h
    FVkDevice.h
    FVkMemoryTracker.h
    FVkBuffer.h
    FVkCommand.h
    FVkPipeline.h
    FVkPipelineCache.h
    FVkMultisampler.h
    FVkTexture.h
    FVkSwapchain.h
    FVkSkybox.h
    FVkFloor.h
    FVkDebugDraw.h
    FVkDepthTarget.h
    FVkOverlayPass.h
    ShadowMapOffsetTexture.h
    PointLightShadowMap.h
    VkHelper.h
    FVkShader.h
    FVkPipelineLayout.h
    DescriptorPoolAllocator.h
    ${VULKAN_SPIRV_REFLECT_PATH}/spirv_reflect.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../External/glm
)

SET(PROJECT_SOURCES
    # Vulkan
    Renderer_Vulkan.cpp
    FVkDevice.cpp
    FVkMemoryTracker.cpp
    FVkBuffer.cpp
    FVkCommand.cpp
    FVkPipeline.cpp
    FVkPipelineCache.cpp
    FVkMultisampler.cpp
    FVkTexture.cpp
    FVkSwapchain.cpp
    FVkSkybox.cpp
    FVkFloor.cpp
    FVkDebugDraw.cpp
    FVkDepthTarget.cpp
    FVkOverlayPass.cpp
    ShadowMapOffsetTexture.cpp
    PointLightShadowMap.cpp
    VkHelper.cpp
    FVkShader.cpp
    FVkPipelineLayout.cpp
    DescriptorPoolAllocator.cpp
     ${VULKAN_SPIRV_REFLECT_PATH}/spirv_reflect.c
)

LIST(APPEND VULKAN_PRIVATE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../
                                        ${CMAKE_CURRENT_SOURCE_DIR})
LIST(APPEND VULKAN_PUBLIC_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../../../External/glm)

# === Create Library ===
ADD_LIBRARY(VulkanBackend STATIC ${PROJECT_HEADERS} ${PROJECT_SOURCES})

TARGET_COMPILE_DEFINITIONS(VulkanBackend
    PRIVATE
        FLEUR_BUILD_LIB
        $<$<CONFIG:Debug>:FL_CONF_DEBUG>
)

TARGET_COMPILE_DEFINITIONS(VulkanBackend PRIVATE ${PRIVATE_COMPILE_DEFINITIONS})

if(MSVC)
    TARGET_COMPILE_OPTIONS(VulkanBackend PRIVATE /utf-8)
endif()

# Vulkan
FIND_PACKAGE(Vulkan REQUIRED)
TARGET_LINK_LIBRARIES(VulkanBackend PRIVATE Vulkan::Vulkan FleurMath)
LIST(APPEND VULKAN_PRIVATE_INCLUDE_DIRS ${VULKAN_SPIRV_REFLECT_PATH})
LIST(APPEND VULKAN_PRIVATE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../../../
                                        ${CMAKE_CURRENT_SOURCE_DIR}/../../../External/spdlog/include)

TARGET_INCLUDE_DIRECTORIES(VulkanBackend PRIVATE ${VULKAN_PRIVATE_INCLUDE_DIRS})
TARGET_INCLUDE_DIRECTORIES(VulkanBackend PUBLIC ${VULKAN_PUBLIC_INCLUDE_DIRS})

# === Target Properties ===
SET_TARGET_PROPERTIES(VulkanBackend PROPERTIES FOLDER "Engine/Lux/Backend")
