# OpenGL Backend

SET(PROJECT_HEADERS
    ../RendererContract.hpp
    Renderer_OpenGL.h
)

SET(PROJECT_SOURCES
    Renderer_OpenGL.cpp
)


# === Create Library ===
ADD_LIBRARY(Renderer_OpenGL_Backend SHARED ${PROJECT_HEADERS} ${PROJECT_SOURCES})

# === Target Properties ===
SET_TARGET_PROPERTIES(Renderer_OpenGL_Backend PROPERTIES FOLDER "Engine/Renderer/Backend")


TARGET_COMPILE_DEFINITIONS(Renderer_OpenGL_Backend PRIVATE "DLL_EXPORT")