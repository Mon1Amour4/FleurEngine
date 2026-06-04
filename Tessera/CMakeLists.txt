# === Tessera — Physics Engine ===

set(PROJECT_HEADERS
    Tessera.h
)

set(PROJECT_SOURCES
    Tessera.cpp
)

add_library(Tessera STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})

target_compile_features(Tessera PUBLIC cxx_std_${FL_CPP_LANG_VER})

target_compile_options(Tessera PRIVATE ${FLEUR_COMPILATION_FLAGS})

target_include_directories(Tessera PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(Tessera PROPERTIES FOLDER "Engine")
