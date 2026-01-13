cmake_minimum_required(VERSION 3.30)
project(derivec_project C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C & C++ Standards
# - Use of `_Generic` means C11 minimum
set(CMAKE_C_STANDARD            23  CACHE STRING "C standard"            )
set(CMAKE_C_STANDARD_REQUIRED   ON  CACHE BOOL   "Require C std"         )
set(CMAKE_C_EXTENSIONS          OFF CACHE BOOL   "No compiler extensions")
set(CMAKE_CXX_STANDARD          23  CACHE STRING "C++ standard"          )
set(CMAKE_CXX_STANDARD_REQUIRED ON  CACHE BOOL   "Require C++ std"       )
set(CMAKE_CXX_EXTENSIONS        OFF CACHE BOOL   "No compiler extensions")

# Build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, defaulting to Debug")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the build type")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Warnings
# Applied to the derive-c (c compiled) examples, and for warning tests.
#  - Not used for tests, as these are not a product of the library, and as rapidcheck and gtest 
#    fail many stricter warnings.
add_library(dc_warnings_c INTERFACE)
target_compile_options(dc_warnings_c INTERFACE
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wall>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wextra>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wpedantic>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Werror>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wformat=2>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wformat-security>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wnull-dereference>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wvla>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wcast-align>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wwrite-strings>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wconversion>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wsign-conversion>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wshadow>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wundef>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wpointer-arith>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wswitch-enum>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wmissing-noreturn>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wimplicit-fallthrough>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wunreachable-code>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wdouble-promotion>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wfloat-equal>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wstrict-aliasing>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wstrict-prototypes>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wmissing-prototypes>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wmissing-declarations>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wno-unused-function>
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wno-extra-semi>

  # We allow for empty structs (required to keep the release overhead of debug 
  # only memory tracking, invalidation tracking zero)
  $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wno-gnu-empty-struct>
)

# Sanitizers
#  - Supporting address,undefined by default. Msan and tsan are enableable separately.
#  - Defaulted only in debug
#
# We only run msan when also instrumenting the stdlib (see the clang_msan.nix toolchain)
# - Additionally, we cannot run with code coverage (which internally fails msan)
#
# By default, asan and ubsan are run, provide the most benefit & are compatible with eachother.
set(_dbg_default OFF)
if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
  set(_dbg_default ON)
endif()

option(USE_ASAN  "Enable AddressSanitizer (ASan)"            ${_dbg_default})
option(USE_UBSAN "Enable UndefinedBehaviorSanitizer (UBSan)" ${_dbg_default})
option(USE_MSAN  "Enable MemorySanitizer (MSan)"             OFF)

if (USE_MSAN AND (USE_ASAN OR USE_UBSAN))
  message(FATAL_ERROR "MSan cannot be enabled together with ASan/UBSan. Use only USE_MSAN=ON.")
endif()

set(_SAN_CSV "")
if (USE_ASAN)
  set(_SAN_CSV "address")
endif()
if (USE_UBSAN)
  if (_SAN_CSV STREQUAL "")
    set(_SAN_CSV "undefined")
  else()
    set(_SAN_CSV "${_SAN_CSV},undefined")
  endif()
endif()

if (NOT _SAN_CSV STREQUAL "")
  add_compile_options(-fsanitize=${_SAN_CSV})
  add_link_options(-fsanitize=${_SAN_CSV})
endif()

if (USE_MSAN)
  add_compile_options(-fsanitize=memory)
  add_link_options(-fsanitize=memory)

  if (COVERAGE)
    message(FATAL_ERROR "msan is not supported with coverage")
  endif()

  # JUSTIFY: Remove optimisations
  # For better stack traces (https://clang.llvm.org/docs/MemorySanitizer.html#how-to-build)
  add_compile_options(
    -fsanitize-memory-track-origins
    -fno-optimize-sibling-calls
  )

  # JUSTIFY: Using a different libc++
  # - All libraries used must be instrumented, including libc++
  # - Here we use variables exported by the clang_msan.nix shell
  set(_MSAN_LIBCXX "$ENV{NIX_MSAN_LIBCXX}")
  set(_MSAN_CXXINC "$ENV{NIX_MSAN_CXXINC}")
  set(_MSAN_LIBDIR "$ENV{NIX_MSAN_LIBDIR}")
  set(_MSAN_RTINC  "$ENV{NIX_MSAN_RTINC}")

  if (NOT _MSAN_LIBCXX OR NOT _MSAN_CXXINC OR NOT _MSAN_LIBDIR OR NOT _MSAN_RTINC)
    message(FATAL_ERROR
      "USE_MSAN=ON requires nix-shell exports:\n"
      "  NIX_MSAN_LIBCXX, NIX_MSAN_CXXINC, NIX_MSAN_LIBDIR, NIX_MSAN_RTINC\n"
      "Missing one or more."
    )
  endif()

  message(STATUS "MSan libc++ prefix: ${_MSAN_LIBCXX}")
  message(STATUS "MSan libc++ headers: ${_MSAN_CXXINC}")
  message(STATUS "MSan libc++ libdir:  ${_MSAN_LIBDIR}")
  message(STATUS "MSan compiler-rt headers: ${_MSAN_RTINC}")

  add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>
    $<$<COMPILE_LANGUAGE:CXX>:-isystem${_MSAN_CXXINC}>
    $<$<COMPILE_LANGUAGE:CXX>:-isystem${_MSAN_RTINC}>
    $<$<COMPILE_LANGUAGE:C>:-isystem${_MSAN_RTINC}>
  )

  add_link_options(
    $<$<LINK_LANGUAGE:CXX>:-stdlib=libc++>
    $<$<LINK_LANGUAGE:CXX>:-L${_MSAN_LIBDIR}>
    $<$<LINK_LANGUAGE:CXX>:-Wl,-rpath,${_MSAN_LIBDIR}>
    $<$<LINK_LANGUAGE:CXX>:-lc++abi>
  )
endif()

if (USE_ASAN OR USE_UBSAN OR USE_MSAN)
  message(STATUS "Sanitizers enabled:"
    "$<$<BOOL:${USE_ASAN}> ASan>"
    "$<$<BOOL:${USE_UBSAN}> UBSan>"
    "$<$<BOOL:${USE_MSAN}> MSan>"
  )
else()
  message(STATUS "No Sanitizers enabled")
endif()

# Memory Tracking
#  - Configured to use msan or asan based on `SANITIZERS`
#  - See: memory_tracker.h
set(CUSTOM_MEMORY_TRACKING 2 CACHE STRING "")
set_property(CACHE CUSTOM_MEMORY_TRACKING PROPERTY STRINGS 0 1 2)
message(STATUS "Memory Tracking at level=${CUSTOM_MEMORY_TRACKING}")
add_compile_definitions(CUSTOM_MEMORY_TRACKING=${CUSTOM_MEMORY_TRACKING})

# Debug Info
# - Want to additionally add macro debug information
# - g1-3 flags supported by both clang and gcc
# - See: https://clang.llvm.org/docs/ClangCommandLineReference.html#debug-level
if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_C_FLAGS_DEBUG   "-g3 -O1 -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O1 -fno-omit-frame-pointer")
  message(STATUS "Additional debug information enabled")
endif()

# Testing
enable_testing()

# Optimisation Reports
option(OPT_REPORTS "Create Clang Optimisation Reports" Off)

# Project Structure
option(TESTS "Build tests" ON)
option(BENCH "Build benchmarks" ON)
option(DOCS "Build documentation" ON)
option(LINTING "Enable linting & formatting" ON)

add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(scripts)

if (TESTS)
  add_subdirectory(test)
endif()

if (BENCH)
  add_subdirectory(bench)
endif()

if (DOCS)
  add_subdirectory(docs)
endif()

if (LINTING)
  # JUSTIFY: No formatting for cmake
  find_program(CLANG_FORMAT NAMES clang-format)
  if (CLANG_FORMAT)
    file(GLOB_RECURSE SOURCE_FILES
      ${CMAKE_SOURCE_DIR}/src/*.h
      ${CMAKE_SOURCE_DIR}/src/*.c
      ${CMAKE_SOURCE_DIR}/src/*.hpp
      ${CMAKE_SOURCE_DIR}/src/*.cpp
      ${CMAKE_SOURCE_DIR}/examples/*.c
      ${CMAKE_SOURCE_DIR}/examples/*.h
      ${CMAKE_SOURCE_DIR}/test/*.cpp
      ${CMAKE_SOURCE_DIR}/test/*.hpp
    )

    add_custom_target(
      format
      COMMAND ${CLANG_FORMAT} -i ${SOURCE_FILES}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Reformatting all source files"
    )

    add_custom_target(
      format-check
      COMMAND ${CLANG_FORMAT} --dry-run --Werror ${SOURCE_FILES}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Checking formatting"
    )

    add_test(
      NAME lint/format-check
      COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target format-check
    )
    set_tests_properties(lint/format-check PROPERTIES LABELS "linting")
  endif()

  find_program(CLANG_TIDY NAMES clang-tidy)

  # JUSTIFY: No clang-tidy for cpp
  #  - Warns for C code included in cpp, produces much noise
  if (CLANG_TIDY)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY}")
    file(GLOB_RECURSE TIDY_C_SRC  ${CMAKE_SOURCE_DIR}/examples/*.c)
    file(GLOB_RECURSE TIDY_C_HDRS ${CMAKE_SOURCE_DIR}/src/*.h ${CMAKE_SOURCE_DIR}/examples/*.h)

    execute_process(
      COMMAND ${CMAKE_C_COMPILER} -print-resource-dir
      OUTPUT_VARIABLE CLANG_RESOURCE_DIR
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (TIDY_C_SRC)
      add_custom_target(lint-c-src
        COMMAND ${CLANG_TIDY}
          -p ${CMAKE_BINARY_DIR}
          --extra-arg=-resource-dir=${CLANG_RESOURCE_DIR}
          ${TIDY_C_SRC}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        VERBATIM
      )
    endif()

    add_custom_target(lint-c-hdrs
      COMMAND ${CLANG_TIDY}
        -p ${CMAKE_BINARY_DIR}
        --extra-arg-before=-xc
        --extra-arg-before=-std=c23
        --extra-arg=-D_GNU_SOURCE # For cookie_io from stdio.h
        --extra-arg=-resource-dir=${CLANG_RESOURCE_DIR}
        --extra-arg=-I${CMAKE_SOURCE_DIR}/src
        --header-filter="^${CMAKE_SOURCE_DIR}/src/"
        ${TIDY_C_HDRS}
        --
        -xc
        -std=c23
        -resource-dir=${CLANG_RESOURCE_DIR}
        -I${CMAKE_SOURCE_DIR}/src
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      VERBATIM
    )

    if (TIDY_C_SRC)
      add_custom_target(lint DEPENDS lint-c-src lint-c-hdrs)
    else()
      add_custom_target(lint DEPENDS lint-c-hdrs)
    endif()

    add_test(
      NAME lint/lint
      COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target lint
    )
    set_tests_properties(lint/lint PROPERTIES LABELS "linting")
  endif()
endif()
