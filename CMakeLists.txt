cmake_minimum_required(VERSION 3.30)
project(derivec_project C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C & C++ Standards
# - Use of `_Generic` means C11 minimum
set(CMAKE_C_STANDARD            23  CACHE STRING "C standard"             FORCE)
set(CMAKE_C_STANDARD_REQUIRED   ON  CACHE BOOL   "Require C std"          FORCE)
set(CMAKE_C_EXTENSIONS          OFF CACHE BOOL   "No compiler extensions" FORCE)
set(CMAKE_CXX_STANDARD          23  CACHE STRING "C++ standard"           FORCE)
set(CMAKE_CXX_STANDARD_REQUIRED ON  CACHE BOOL   "Require C++ std"        FORCE)
set(CMAKE_CXX_EXTENSIONS        OFF CACHE BOOL   "No compiler extensions" FORCE)

# Build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, defaulting to Debug")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Sanitizers
#  - Supporting address,undefined by default. Msan and tsan are be enableable separately.
#  - Defaulted only in debug
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEFAULT_SANITIZERS "address,undefined")
else()
  set(DEFAULT_SANITIZERS "")
endif()

# JUSTIFY: Custom tracking off for gcc
#  - the readme.c example dies with and uknown crash inside memset on GCC.
#  TODO(oliverkillane): Figure out why this happens.
# `SUMMARY: AddressSanitizer: unknown-crash (/nix/store/g73qbrh4mpxymzwq0fzdd3rhvn6jwn5m-gcc-14-20241116-lib/lib/libasan.so.8+0xf99db) in memset`
if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(DEFAULT_CUSTOM_MEMORY_TRACKING ON)
else()
    set(DEFAULT_CUSTOM_MEMORY_TRACKING OFF)
endif()

option(CUSTOM_MEMORY_TRACKING "Enable custom memory tracking" ${DEFAULT_CUSTOM_MEMORY_TRACKING})
if (CUSTOM_MEMORY_TRACKING)
  add_definitions(-DCUSTOM_MEMORY_TRACKING)
endif()


set(SANITIZERS "${DEFAULT_SANITIZERS}" CACHE STRING "Sanitizers to use" FORCE)
set_property(CACHE SANITIZERS PROPERTY STRINGS "address,undefined" "memory" "thread")
if (SANITIZERS)

  add_compile_options(-fsanitize=${SANITIZERS})
  add_link_options(-fsanitize=${SANITIZERS})
  message(STATUS "Using Sanitizers: ${SANITIZERS}")
else()
  message(STATUS "No Sanitizers enabled")
endif()


# Debug Info
# - Want to additionally add macro debug information
# - g1-3 flags supported by both clang and gcc
# - See: https://clang.llvm.org/docs/ClangCommandLineReference.html#debug-level
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS_DEBUG   "-g3 -O1")
    set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O1")
    message(STATUS "Additional debug information enabled")
endif()

# Testing
# - Including for the examples directory
enable_testing()

# Project Structure
option(TESTS "Build tests" ON)
option(BENCH "Build benchmarks" ON)
option(DOCS "Build documentation" ON)

add_subdirectory(src)
add_subdirectory(examples)
if (TESTS)
    add_subdirectory(test)
endif()

if(BENCH)
   add_subdirectory(bench)
endif()

if(DOCS)
   add_subdirectory(docs)
endif()

# Linting
option(LINTING "Enable linting & formatting" ON)

if(LINTING)
    # JUSTIFY: No formatting for cmake
    # - Could use cmakefmt, or add python to the project and use cmake_format
    # - low popularity of both tools & likelihood of formatting issues later / cmake 
    #   complexity
    find_program(CLANG_FORMAT NAMES clang-format)
    if(CLANG_FORMAT)
        file(GLOB_RECURSE SOURCE_FILES
            ${CMAKE_SOURCE_DIR}/src/*.h
            ${CMAKE_SOURCE_DIR}/src/*.c
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
          COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target format-check)
        set_tests_properties(lint/format-check PROPERTIES LABELS "linting")
    endif()

    find_program(CLANG_TIDY NAMES clang-tidy)

    # JUSTIFY: No clang-tidy for cpp
    #  - Warns for C code included in cpp, produces much noise
    if(CLANG_TIDY)
      message(STATUS "Found clang-tidy: ${CLANG_TIDY}")
      file(GLOB_RECURSE TIDY_C_SRC  ${CMAKE_SOURCE_DIR}/examples/*.c)
      file(GLOB_RECURSE TIDY_C_HDRS ${CMAKE_SOURCE_DIR}/src/*.h ${CMAKE_SOURCE_DIR}/examples/*.h)

      execute_process(
        COMMAND ${CMAKE_C_COMPILER} -print-resource-dir
        OUTPUT_VARIABLE CLANG_RESOURCE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )

      if(TIDY_C_SRC)
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
          --extra-arg=-resource-dir=${CLANG_RESOURCE_DIR} 
          --extra-arg=-I${CMAKE_SOURCE_DIR}/src 
          --header-filter="^${CMAKE_SOURCE_DIR}/src/" 
          ${TIDY_C_HDRS}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} 
        VERBATIM
      )

      if(TIDY_C_SRC)
        add_custom_target(lint DEPENDS lint-c-src lint-c-hdrs)
      else()
        add_custom_target(lint DEPENDS lint-c-hdrs)
      endif()

      add_test(
        NAME lint/lint 
        COMMAND ${CMAKE_COMMAND} 
            --build ${CMAKE_BINARY_DIR} 
            --target lint
      )
      set_tests_properties(lint/lint PROPERTIES LABELS "linting")
    endif()
endif()