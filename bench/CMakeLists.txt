include(FetchContent)
include(CTest)

FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.4
)

# We only consume the library; disable googlebenchmark's own tests (incl asm tests)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_ASSEMBLY_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googlebenchmark)

# Build one executable per bench.cpp, anywhere under this directory
file(GLOB_RECURSE BENCH_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/**/bench.cpp")

foreach(BENCH_FILE ${BENCH_SOURCES})
  # Path relative to this bench/ directory
  file(RELATIVE_PATH REL "${CMAKE_CURRENT_SOURCE_DIR}" "${BENCH_FILE}") # e.g. "containers/vector/bench.cpp"
  get_filename_component(REL_DIR "${REL}" DIRECTORY)                    # e.g. "containers/vector"
  get_filename_component(REL_BASE "${REL}" NAME_WE)                     # "bench"

  # Unique, stable target name: bench_containers_vector
  set(BENCH_TARGET ${REL_DIR})
  string(REPLACE "/" "_" BENCH_TARGET "${BENCH_TARGET}")
  string(REPLACE "\\" "_" BENCH_TARGET "${BENCH_TARGET}") # Windows safety

  add_executable(${BENCH_TARGET} "${BENCH_FILE}")
  target_link_libraries(${BENCH_TARGET} PRIVATE benchmark::benchmark derivec derivecpp)

  # Optional: keep all bench binaries together
  set_target_properties(${BENCH_TARGET} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bench"
  )

  # Register as a ctest test
  add_test(
    NAME ${BENCH_TARGET}
    COMMAND ${BENCH_TARGET}
            --benchmark_min_time=0.01
            --benchmark_repetitions=1
            --benchmark_report_aggregates_only=true
            --benchmark_format=console
  )

  # Labels:
  # - always "benchmark"
  # - plus each path component so you can filter with -L containers, -L vector, etc.
  set(TEST_LABELS "benchmark")
  if (NOT REL_DIR STREQUAL "")
    string(REPLACE "/" ";" REL_DIR_PARTS "${REL_DIR}")
    list(APPEND TEST_LABELS ${REL_DIR_PARTS})
  endif()

  set_tests_properties(${BENCH_TARGET} PROPERTIES
    LABELS "${TEST_LABELS}"
    TIMEOUT 120
  )
endforeach()
