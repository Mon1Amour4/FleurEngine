# Protobuf

# Flags configuration:
set(protobuf_BUILD_TESTS OFF)
set(protobuf_WITH_ZLIB OFF)
set(BUILD_SHARED_LIBS OFF)
set(_ITERATOR_DEBUG_LEVEL 0)
set(ABSL_RUN_TESTS OFF)
set(BUILD_TESTING OFF)
set(protobuf_ABSL_PROVIDER package)
#set(DCMAKE_CXX_STANDARD ${CPP_LANG_VER})
set(protobuf_BUILD_LIBUPB ON)
set(protobuf_BUILD_CONFORMANCE OFF)
set(protobuf_INSTALL ON)
set(protobuf_MSVC_STATIC_RUNTIME OFF)
set(protobuf_BUILD_EXAMPLES OFF)
set(protobuf_BUILD_PROTOBUF_BINARIES ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

# Paths:
set(PROT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/External/protobuf)
set(absl_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../build/${FUEGO_PLATFORM}/Engine/External/abseil )

add_subdirectory(${PROT_ROOT})

get_directory_property(PROT_TARGETS DIRECTORY ${PROT_ROOT} BUILDSYSTEM_TARGETS)
    foreach(target ${PROT_TARGETS})
        set_target_properties(${target} PROPERTIES FOLDER "3rd party/Protobuf")
    endforeach()
set_target_properties(utf8_range utf8_validity PROPERTIES FOLDER "3rd party/Protobuf")

# Proto
GetProtoHeaderFiles(PROTO_OUT ${PROTO_OUTPUT_FOLDER})
GetProtoSourceFiles(PROTO_OUT ${PROTO_OUTPUT_FOLDER})
