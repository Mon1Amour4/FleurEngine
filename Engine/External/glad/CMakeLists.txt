# === Glad ===

SET(PROJECT_HEADERS 
        include/glad/wgl.h
        include/KHR/khrplatform.h
)

SET(PROJECT_SOURCES 
        src/gl.c
        src/wgl.c
)

# === Create Library ===
ADD_LIBRARY(Glad STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# === Link Libraries ===
TARGET_LINK_LIBRARIES(Glad PRIVATE opengl32)

# === Include Directories ===
TARGET_INCLUDE_DIRECTORIES(Glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# === Target Properties ===
SET_TARGET_PROPERTIES(Glad PROPERTIES LANGUAGES C)
