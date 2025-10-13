MESSAGE(STATUS "Fleur tests are ENABLED")

include(../External/googletest.cmake)
# Googletest configuration
SET(FLEUR_ROOT ${CMAKE_SOURCE_DIR})
SET(GOOGLE_TEST_ROOT ${FLEUR_ROOT}/External/googletest)
SET(GOOGLE_TEST_BUILD ${FLEUR_ROOT}/build/${FLEUR_PLATFORM}/External/googletest)

# Tests project configuration:
SET(COMMON_TARGET_PROPERTIES PROPERTIES FOLDER "Tests")
# End of Tests configuration

# CoreLibtests configuration:
SET(CORE_LIB_TESTS_ROOT ${FLEUR_ROOT}/CoreLib)
SET(CORE_LIB_TESTS_BUILD ${FLEUR_ROOT}/build/${FLEUR_PLATFORM}/CoreLibTests)
# End of CoreLibtests configuration

# Main part:
SET(SRC_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/src)
SET(INCLUDE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/include)

SET(TESTS_HEADERS 
  ${INCLUDE_FOLDER}/Tests.h 
  ${INCLUDE_FOLDER}/CoreLibTests.h 
  ${INCLUDE_FOLDER}/MemoryManagerTests.h
  ${INCLUDE_FOLDER}/SyntheticTypes.hpp
  ${INCLUDE_FOLDER}/ConceptTests.hpp)

SET(TESTS_SOURCES ${SRC_FOLDER}/Tests.cpp)

ADD_EXECUTABLE(FleurTests ${TESTS_HEADERS} ${TESTS_SOURCES})

SET_TARGET_PROPERTIES(FleurTests ${COMMON_TARGET_PROPERTIES})

TARGET_LINK_LIBRARIES(FleurTests PUBLIC 
  CoreLib 
  GTest::gtest 
  FleurEngine 
  FleurInterface 
  MemoryManager)

TARGET_INCLUDE_DIRECTORIES(FleurTests PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}/include 
  ${GOOGLE_TEST_ROOT}/googletest/include 
  ${CORE_LIB_TESTS_ROOT}
  ../Engine/Fleur/MemoryManager)

TARGET_COMPILE_FEATURES(FleurTests PUBLIC cxx_std_20)
