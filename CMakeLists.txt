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

set(SANITIZERS "${DEFAULT_SANITIZERS}" CACHE STRING "Sanitizers to use" FORCE)
set_property(CACHE SANITIZERS PROPERTY STRINGS "address,undefined" "memory" "thread")
if (SANITIZERS)

  add_compile_options(-fsanitize=${SANITIZERS})
  add_link_options(-fsanitize=${SANITIZERS})
  message(STATUS "Using Sanitizers: ${SANITIZERS}")
else()
  message(STATUS "No Sanitizers enabled")
endif()


# Memory Tracking
#  - Configured to use msan or asan based on `SANITIZERS`
#  - See: memory_tracker.h
# Default tracking level (see memory_tracker.h for details)
set(CUSTOM_MEMORY_TRACKING 2 CACHE STRING "")
set_property(CACHE CUSTOM_MEMORY_TRACKING PROPERTY STRINGS 0 1 2)
message(STATUS "Memory Tracking at level=${CUSTOM_MEMORY_TRACKING}")
add_compile_definitions(CUSTOM_MEMORY_TRACKING=${CUSTOM_MEMORY_TRACKING})

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

if(BENCH)
   add_subdirectory(bench)
endif()

if(DOCS)
   add_subdirectory(docs)
endif()

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
          --extra-arg-before=-std=c23
          --extra-arg=-D_GNU_SOURCE # For cookie_io from stdio.h
          --extra-arg=-resource-dir=${CLANG_RESOURCE_DIR} 
          --extra-arg=-I${CMAKE_SOURCE_DIR}/src 
          --header-filter="^${CMAKE_SOURCE_DIR}/src/" 
          ${TIDY_C_HDRS}
          --                          # everything after this is appended to the FE
          -xc                         # force C language (wins because it's last)
          -std=c23                    # match your C standard
          -resource-dir=${CLANGC_RESOURCE_DIR}
          -I${CMAKE_SOURCE_DIR}/src
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