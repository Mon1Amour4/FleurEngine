message(STATUS "Fleur tests are ENABLED")

include(../External/googletest.cmake)
include(GoogleTest)

set(SRC_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Shared helper headers (no TEST cases live here).
set(TESTS_HEADERS
    ${INCLUDE_FOLDER}/SyntheticTypes.hpp
)

# Each suite is its own translation unit so they compile in parallel and stay
# isolated (no shared TU / ODR coupling). main() lives in Tests.cpp.
set(TESTS_SOURCES
    ${SRC_FOLDER}/Tests.cpp
    ${SRC_FOLDER}/CoreLibTests.cpp
    ${SRC_FOLDER}/MathTests.cpp
    ${SRC_FOLDER}/DirectionalShadowMathTests.cpp
    ${SRC_FOLDER}/CoreLibEdgeTests.cpp
    ${SRC_FOLDER}/ConceptTests.cpp
    ${SRC_FOLDER}/AssetCacheTests.cpp
    ${SRC_FOLDER}/AssetCacheEdgeTests.cpp
    ${SRC_FOLDER}/MemoryManagerTests.cpp
    ${SRC_FOLDER}/BitSet64Tests.cpp
    ${SRC_FOLDER}/PageAllocatorTests.cpp
    ${SRC_FOLDER}/DeathTests.cpp
    ${SRC_FOLDER}/ContractTests.cpp
    ${SRC_FOLDER}/AllocatorInvariantTests.cpp
    ${SRC_FOLDER}/SmartPtrAllocatorTests.cpp
    ${SRC_FOLDER}/SmallEdgeTests.cpp
    ${SRC_FOLDER}/AllocatorSoakTests.cpp
    ${SRC_FOLDER}/LightingSystemTests.cpp
    # MemoryManager .natvis file
    ${SRC_FOLDER}/../../MemoryManager/MemoryManager.natvis
)

add_executable(FleurTests ${TESTS_HEADERS} ${TESTS_SOURCES})

set_target_properties(FleurTests PROPERTIES FOLDER "Tests")

target_link_libraries(FleurTests PUBLIC
    CoreLib
    GTest::gtest
    FleurEngine
    FleurInterface
)
# CoreLib brings MemoryManager transitively.

target_include_directories(FleurTests PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
# CoreLib / MemoryManager / Engine include paths arrive transitively via target_link_libraries.

target_compile_features(FleurTests PUBLIC cxx_std_${FL_CPP_LANG_VER})

# Register every TEST as an individual ctest case (parallel, granular reporting).
# Benchmarks/stress cases are excluded from the default ctest set; run them
# explicitly via FleurTests --gtest_filter=... when needed.
gtest_discover_tests(FleurTests
    TEST_FILTER -*Benchmark*:*Random*:*Fixed*:*TLSF*:*Chunks*:*ArrayAllocations*
    DISCOVERY_MODE PRE_TEST
)

# Benchmark/stress executable. EXCLUDE_FROM_ALL so normal builds skip it; build
# on demand only (cmake --build ... --target FleurBenchmarks). Has its own main()
# and is intentionally NOT registered with gtest_discover_tests (stays out of ctest).
add_executable(FleurBenchmarks EXCLUDE_FROM_ALL ${SRC_FOLDER}/../benchmarks/MemoryManagerBench.cpp)

set_target_properties(FleurBenchmarks PROPERTIES FOLDER "Tests")

target_link_libraries(FleurBenchmarks PUBLIC
    GTest::gtest
    MemoryManager
    CoreLib
    FleurInterface
)

target_include_directories(FleurBenchmarks PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(FleurBenchmarks PUBLIC cxx_std_${FL_CPP_LANG_VER})

# AddressSanitizer build of the allocator + smart-pointer tests. Compiles the
# MemoryManager sources directly (rather than linking the prebuilt lib) so ASan
# instruments first-party allocator code WITHOUT dragging the Vulkan/TBB/ImGui
# stack through the sanitizer. EXCLUDE_FROM_ALL: build on demand with
#   cmake --build <dir> --target FleurTestsAsan --config Debug
# then run the exe with the MSVC asan runtime dir on PATH.
# NOTE: MSVC ASan on Windows has no LeakSanitizer, so pure leaks are not caught;
# it does catch heap/stack buffer overflow and use-after-free.
if(MSVC)
  # Allocator-only: MemoryManager is dependency-free, so it isolates cleanly.
  # CoreLib smart pointers pull in Engine headers, so they are not included here;
  # ASan over CoreLib/AssetCache would need a full-engine instrumented build.
  add_executable(FleurTestsAsan EXCLUDE_FROM_ALL
      ${SRC_FOLDER}/Tests.cpp
      ${SRC_FOLDER}/BitSet64Tests.cpp
      ${SRC_FOLDER}/PageAllocatorTests.cpp
      ${SRC_FOLDER}/MemoryManagerTests.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/../MemoryManager/MemoryManager.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/../MemoryManager/BitSet64.cpp
  )

  target_link_libraries(FleurTestsAsan PRIVATE GTest::gtest)

  target_include_directories(FleurTestsAsan PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/../MemoryManager
  )

  # MemoryManager.cpp references MemoryInfo, which the headers gate behind this.
  target_compile_definitions(FleurTestsAsan PRIVATE MEMORYMANAGER_PROFILING)

  target_compile_features(FleurTestsAsan PRIVATE cxx_std_${FL_CPP_LANG_VER})
  target_compile_options(FleurTestsAsan PRIVATE /fsanitize=address /Zi)
  # /fsanitize=address belongs on compilation. Passing it directly to
  # link.exe produces LNK4044 with the Visual Studio generator.
  target_link_options(FleurTestsAsan PRIVATE /INCREMENTAL:NO)
  set_target_properties(FleurTestsAsan PROPERTIES FOLDER "Tests")
endif()
