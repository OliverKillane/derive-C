include(FetchContent)
include(CTest)

set(GOOGLE_BENCHMARK_VERSION v1.9.4)
set(UNORDERED_DENSE_VERSION v4.8.1)
set(ABSEIL_VERSION 20240722.0)
set(BOOST_VERSION 1.86.0)

FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG "${GOOGLE_BENCHMARK_VERSION}"
)

# We only consume the library; disable googlebenchmark's own tests (incl asm tests)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_ASSEMBLY_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googlebenchmark)

FetchContent_Declare(
  unordered_dense
  GIT_REPOSITORY https://github.com/martinus/unordered_dense.git
  GIT_TAG "${UNORDERED_DENSE_VERSION}"
)
FetchContent_MakeAvailable(unordered_dense)

FetchContent_Declare(
  abseil
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG "${ABSEIL_VERSION}"
)
set(ABSL_PROPAGATE_CXX_STD ON)
FetchContent_MakeAvailable(abseil)

FetchContent_Declare(
  boost
  URL https://github.com/boostorg/boost/releases/download/boost-${BOOST_VERSION}/boost-${BOOST_VERSION}-cmake.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(BOOST_INCLUDE_LIBRARIES unordered)
FetchContent_MakeAvailable(boost)

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
  target_link_libraries(${BENCH_TARGET} PRIVATE 
    benchmark::benchmark 
    derivec 
    derivecpp
    unordered_dense::unordered_dense
    absl::flat_hash_map
    Boost::unordered
  )

  # Optional: keep all bench binaries together
  set_target_properties(${BENCH_TARGET} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bench"
  )

  # Register as a ctest test
  add_test(
    NAME ${BENCH_TARGET}
    COMMAND ${BENCH_TARGET}
            --benchmark_min_time=0.01s
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
    TIMEOUT 360
  )

  # Track all benchmark targets for the 'run-benchmarks' target
  list(APPEND ALL_BENCH_TARGETS ${BENCH_TARGET})
endforeach()

# Create a target to run all benchmarks and collect JSON output
set(BENCHMARK_OUTPUT_DIR ${CMAKE_BINARY_DIR}/benchmark_results)

add_custom_target(run-benchmarks
  DEPENDS ${ALL_BENCH_TARGETS}
  COMMENT "📊 Running all benchmarks and collecting JSON output"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}

  COMMAND ${CMAKE_COMMAND} -E remove_directory ${BENCHMARK_OUTPUT_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${BENCHMARK_OUTPUT_DIR}

  VERBATIM
  USES_TERMINAL
)

# Add individual benchmark execution commands
foreach(BENCH_TARGET ${ALL_BENCH_TARGETS})
  add_custom_command(
    TARGET run-benchmarks
    POST_BUILD
    COMMAND ${CMAKE_BINARY_DIR}/bench/${BENCH_TARGET}
            --benchmark_format=json
            --benchmark_out=${BENCHMARK_OUTPUT_DIR}/${BENCH_TARGET}.json
            --benchmark_min_time=0.1s
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running ${BENCH_TARGET}..."
    VERBATIM
  )
endforeach()
