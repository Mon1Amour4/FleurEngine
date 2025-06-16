FUNCTION(GetProtoFiles)
  SET(GLOB_EXPRESSION "*.proto")

  CMAKE_PARSE_ARGUMENTS(ARG "" "" "PATHS" ${ARGN})

  IF(NOT DEFINED ARG_PATHS)
    MESSAGE(WARNING "No Proto files folder paths provided!")
    RETURN()
  ENDIF()

  MESSAGE(STATUS "Proto files folders: ${ARG_PATHS}")

  SET(PROTO_FILES "")

  FOREACH(proto_path ${ARG_PATHS})
    FILE(GLOB proto_files "${proto_path}/${GLOB_EXPRESSION}")

    FOREACH(proto ${proto_files})
      MESSAGE(STATUS "Found proto file: ${proto}")
    ENDFOREACH()

    LIST(APPEND PROTO_FILES ${proto_files})
  ENDFOREACH()

  SET(PROTO_FILES
      ${PROTO_FILES}
      PARENT_SCOPE
  )
ENDFUNCTION()

FUNCTION(GetProtoHeaderFiles)
  SET(GLOB_EXPRESSION "*.pb.h")
  SET(OUT_HEADER_FILES "")

  CMAKE_PARSE_ARGUMENTS(ARG "" "PROTO_OUT" "" ${ARGN})

  IF(NOT DEFINED ARG_PROTO_OUT)
    MESSAGE(WARNING "No proto output folder provided!")
    RETURN()
  ENDIF()

  FILE(GLOB HEADER_FILES ${ARG_PROTO_OUT}/${GLOB_EXPRESSION})
  FOREACH(header ${HEADER_FILES})
    MESSAGE(STATUS "Proto header file: ${header}")
    FILE(TO_CMAKE_PATH "${header}" header)
    LIST(APPEND OUT_HEADER_FILES ${header})
  ENDFOREACH()
  SET(OUT_HEADER_FILES
      ${OUT_HEADER_FILES}
      CACHE INTERNAL "Proto header files"
  )
  RETURN()
ENDFUNCTION()

FUNCTION(GetProtoSourceFiles)
  SET(GLOB_EXPRESSION "*.pb.cc")
  SET(OUT_SOURCE_FILES "")

  CMAKE_PARSE_ARGUMENTS(ARG "" "PROTO_OUT" "" ${ARGN})
  IF(NOT DEFINED ARG_PROTO_OUT)
    MESSAGE(WARNING "No proto output folder provided!")
    RETURN()
  ENDIF()

  FILE(GLOB SOURCE_FILES ${ARG_PROTO_OUT}/${GLOB_EXPRESSION})
  FOREACH(source ${SOURCE_FILES})
    MESSAGE(STATUS "Proto source file: ${source}")
    FILE(TO_CMAKE_PATH "${source}" source)
    LIST(APPEND OUT_SOURCE_FILES ${source})
  ENDFOREACH()
  SET(OUT_SOURCE_FILES
      ${OUT_SOURCE_FILES}
      CACHE INTERNAL "Proto header files"
  )
  RETURN()
ENDFUNCTION()
