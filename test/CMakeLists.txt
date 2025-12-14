include(FetchContent)

# ------------------------------------------------------------------------------
# Coverage
#  - Only setup for clang, testing done with clang for better sanitizer support
# ------------------------------------------------------------------------------
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  option(COVERAGE "Enable code coverage instrumentation" ON)
else()
  # Define it in the cache but hard-set to OFF so user can't override
  set(COVERAGE OFF CACHE BOOL "Enable code coverage instrumentation" FORCE)
endif()

if(COVERAGE)
  set(COVERAGE_FLAGS
    -fprofile-instr-generate
    -fcoverage-mapping
  )
  add_compile_options(${COVERAGE_FLAGS})
  add_link_options(${COVERAGE_FLAGS})
  message(STATUS "Enabling Clang/LLVM coverage flags")
endif()

# ------------------------------------------------------------------------------
# Common compile options
# ------------------------------------------------------------------------------
# JUSTIFY: Deprecated declarations off
#  - Rapidcheck contains cpp23 deprecated features
add_compile_options(-Wno-deprecated-declarations)

# ------------------------------------------------------------------------------
# Externals
# ------------------------------------------------------------------------------
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

set(RC_ENABLE_GTEST ON CACHE BOOL "Rapidcheck GTest Support" FORCE)
FetchContent_Declare(
  rapidcheck
  GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
  GIT_TAG ff6af6fc683159deb51c543b065eba14dfcf329b # Latest as of 3/30/2025
)
FetchContent_MakeAvailable(rapidcheck)

include(GoogleTest)

# ------------------------------------------------------------------------------
# Discover test / fuzz sources (deep)
# ------------------------------------------------------------------------------
file(GLOB_RECURSE _ALL_TEST_CPP CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

set(TEST_CPP_FILES "")
set(FUZZ_CPP_FILES "")
foreach(p IN LISTS _ALL_TEST_CPP)
  get_filename_component(name "${p}" NAME)
  if(name STREQUAL "test.cpp")
    list(APPEND TEST_CPP_FILES "${p}")
  elseif(name STREQUAL "fuzz.cpp")
    list(APPEND FUZZ_CPP_FILES "${p}")
  endif()
endforeach()
unset(_ALL_TEST_CPP)

list(SORT TEST_CPP_FILES)
list(SORT FUZZ_CPP_FILES)

# All binaries that should be included in coverage
set(ALL_TEST_EXECUTABLES)

# Helper: derive a stable, Ninja-safe target name from a source path
function(_derive_test_target_name out_var src_file)
  file(RELATIVE_PATH rel "${CMAKE_CURRENT_SOURCE_DIR}" "${src_file}")
  string(REPLACE "/" "_" safe "${rel}")
  string(REPLACE "." "_" safe "${safe}")
  set(${out_var} "${safe}" PARENT_SCOPE)
endfunction()

# ------------------------------------------------------------------------------
# Tests (test.cpp) — googletest only
# ------------------------------------------------------------------------------
foreach(TEST_FILE IN LISTS TEST_CPP_FILES)
  _derive_test_target_name(TEST_TARGET "${TEST_FILE}")

  file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${TEST_FILE}")
  get_filename_component(test_hierarchy "${REL_PATH}" DIRECTORY)

  add_executable(${TEST_TARGET} "${TEST_FILE}")
  set_target_properties(${TEST_TARGET} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT_NAME "${TEST_TARGET}"
  )

  target_compile_definitions(${TEST_TARGET} PRIVATE ENABLE_MOCKING)

  target_link_libraries(${TEST_TARGET}
    PRIVATE
      derivec
      derivecpp
      gtest
      gmock
      gtest_main
  )

  gtest_discover_tests(${TEST_TARGET}
    TEST_PREFIX "tests/${test_hierarchy}/"
    DISCOVERY_TIMEOUT 30
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    PROPERTIES LABELS "correctness"
  )

  list(APPEND ALL_TEST_EXECUTABLES ${TEST_TARGET})
endforeach()

# ------------------------------------------------------------------------------
# Fuzz tests (fuzz.cpp) — googletest + rapidcheck
# ------------------------------------------------------------------------------
foreach(FUZZ_FILE IN LISTS FUZZ_CPP_FILES)
  _derive_test_target_name(FUZZ_TARGET "${FUZZ_FILE}")

  file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${FUZZ_FILE}")
  get_filename_component(fuzz_hierarchy "${REL_PATH}" DIRECTORY)

  add_executable(${FUZZ_TARGET} "${FUZZ_FILE}")
  set_target_properties(${FUZZ_TARGET} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT_NAME "${FUZZ_TARGET}"
  )

  target_compile_definitions(${FUZZ_TARGET} PRIVATE ENABLE_MOCKING)

  target_link_libraries(${FUZZ_TARGET}
    PRIVATE
      derivec
      derivecpp
      gtest
      gmock
      gtest_main
      rapidcheck
      rapidcheck_gtest
  )

  gtest_discover_tests(${FUZZ_TARGET}
    TEST_PREFIX "fuzz/${fuzz_hierarchy}/"
    DISCOVERY_TIMEOUT 30
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    PROPERTIES LABELS "correctness"
    ENVIRONMENT
      "RC_PARAMS=max_success=2000"
  )

  list(APPEND ALL_TEST_EXECUTABLES ${FUZZ_TARGET})
endforeach()

# ------------------------------------------------------------------------------
# Coverage target
# ------------------------------------------------------------------------------
if(COVERAGE)
  find_program(LLVM_COV_EXECUTABLE NAMES llvm-cov)
  find_program(LLVM_PROFDATA_EXECUTABLE NAMES llvm-profdata)

  set(COVERAGE_OBJECTS)
  foreach(TEST_EXEC IN LISTS ALL_TEST_EXECUTABLES)
    list(APPEND COVERAGE_OBJECTS
      "--object=${CMAKE_CURRENT_BINARY_DIR}/${TEST_EXEC}"
    )
  endforeach()

  set(COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)
  set(COVERAGE_ARGS
    --instr-profile=${COVERAGE_DIR}/coverage.profdata
    "--ignore-filename-regex=(^|.*/)(test|build)/.*"
  )

  if(LLVM_COV_EXECUTABLE AND LLVM_PROFDATA_EXECUTABLE)
    add_custom_target(coverage
      DEPENDS ${ALL_TEST_EXECUTABLES}
      COMMENT "🔍 Running llvm-cov → HTML coverage report (Clang/LLVM)"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}

      COMMAND ${CMAKE_COMMAND} -E remove_directory ${COVERAGE_DIR}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR}

      COMMAND ${CMAKE_COMMAND}
        -E env
          LLVM_PROFILE_FILE=${COVERAGE_DIR}/coverage_%p_%m.profraw
        ${CMAKE_CTEST_COMMAND}
          -L correctness
          -j
          --output-on-failure

      # JUSTIFY: Shell scripting from CMake
      COMMAND /bin/sh -c
        "${LLVM_PROFDATA_EXECUTABLE} merge -sparse ${COVERAGE_DIR}/*.profraw -o ${COVERAGE_DIR}/coverage.profdata"

      COMMAND ${LLVM_COV_EXECUTABLE}
        report
        ${COVERAGE_OBJECTS}
        ${COVERAGE_ARGS}

      COMMAND ${LLVM_COV_EXECUTABLE}
        show
        ${COVERAGE_OBJECTS}
        ${COVERAGE_ARGS}
        --format=html
        --show-branches=count
        --show-line-counts
        --show-regions
        --output-dir=${COVERAGE_DIR}/coverage_html

      VERBATIM
      USES_TERMINAL
    )
  endif()
endif()
