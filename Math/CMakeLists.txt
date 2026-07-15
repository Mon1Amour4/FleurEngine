# === FleurMath — public math facade ===
#
# This target is intentionally independent from Engine so that CoreLib,
# Vulkan, OpenGL, and higher-level engine code can share the same public math
# contract. GLM is an implementation detail of this target.

add_library(FleurMath INTERFACE)

target_include_directories(FleurMath
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/Engine/External/glm
)

target_compile_features(FleurMath INTERFACE cxx_std_${FL_CPP_LANG_VER})

set_target_properties(FleurMath PROPERTIES FOLDER "Core")
