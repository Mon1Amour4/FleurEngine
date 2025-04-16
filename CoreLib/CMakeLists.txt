# CoreLib

set(CMAKE_CXX_STANDARD 20)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(CORELIB_HEADERS
    ${ROOT}/CoreLibConcepts.h ${ROOT}/Uptr.h ${ROOT}/Sptr.h
    ${ROOT}/singleton.hpp ${ROOT}/DefaultAllocator.h)

set(CORELIB_SOURCES ${ROOT}/CoreLib.cpp)
set(CORELIB_BULD ${ROOT}/../build/${FUEGO_PLATFORM}/CoreLibTests)

set(OUTPUT_DIR ${CORELIB_BULD}/)

add_library(CoreLib STATIC ${CORELIB_SOURCES} ${CORELIB_HEADERS})

target_link_libraries(CoreLib PUBLIC GTest::gtest FuegoInterface)

target_include_directories(
  CoreLib PUBLIC ${ROOT}/../External/googletest/googletest/include)
target_include_directories(CoreLib
                           PUBLIC ${GOOGLE_TEST_ROOT}/googletest/include)
