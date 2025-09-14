cmake_minimum_required(VERSION 3.30)
project(derivec_project C CXX)

# Export compilation commands for clang-tidy and other tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# NOTE: Want latest cpp for testing/benchmark, but the derive-c library to be usable for C99 codebases

set( CMAKE_C_STANDARD            23           CACHE STRING "C standard"        FORCE )
set( CMAKE_C_STANDARD_REQUIRED   ON           CACHE BOOL   "Require C std"     FORCE )
set( CMAKE_C_EXTENSIONS          OFF          CACHE BOOL   "No compiler extensions" FORCE )

set( CMAKE_CXX_STANDARD          23           CACHE STRING "C++ standard"      FORCE )
set( CMAKE_CXX_STANDARD_REQUIRED ON           CACHE BOOL   "Require C++ std"   FORCE )
set( CMAKE_CXX_EXTENSIONS        OFF          CACHE BOOL   "No compiler extensions" FORCE )

option(DEVELOP "Enable debug + coverage + sanitizers build" ON)
option(EXTERNALS "Include targets using externals" ON)

if (DEVELOP)
    # NOTE: not compatible with msan and tsan, but can be easily set by passing CMAKE_{C/CXX}_FLAGS
    message(STATUS "Sanitizers enabled: ASAN & UBSAN")
    set(SANITIZER_FLAGS "-fsanitize=address,undefined")

    message(STATUS "Debug symbols on (including for macros)")
    set(CMAKE_C_FLAGS_DEBUG "-g3 -O1")
    set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O1")

    message(STATUS "DEVELOP flag set: Forcing CMAKE_BUILD_TYPE=Debug")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)

    add_compile_options(${SANITIZER_FLAGS})
    add_link_options(${SANITIZER_FLAGS})
endif()

enable_testing()

add_subdirectory(src)
add_subdirectory(examples)

if (EXTERNALS)
    add_subdirectory(test)
    add_subdirectory(bench)
    add_subdirectory(docs)
endif()

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

    file(GLOB_RECURSE CMAKE_FILES
        ${CMAKE_SOURCE_DIR}/CMakeLists.txt
        ${CMAKE_SOURCE_DIR}/*.cmake
    )

    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT} -i ${SOURCE_FILES}
        COMMAND ${CMAKE_COMMAND} --format ${CMAKE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Reformatting all source files"
    )

    add_custom_target(
        format-check
        COMMAND ${CLANG_FORMAT}
                --dry-run
                --Werror
                ${SOURCE_FILES}
        COMMAND ${CMAKE_COMMAND} --format --check ${CMAKE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Verifying clang-format compliance (fails if any file needs reformatting)"
    )

    add_test(NAME format-check COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target format-check)
    set_tests_properties(format-check PROPERTIES LABELS "linting")
endif()

find_program(CLANG_TIDY NAMES clang-tidy)

if(CLANG_TIDY)
  message(STATUS "Found clang-tidy: ${CLANG_TIDY}")
  file(GLOB_RECURSE TIDY_C_SRC  "${CMAKE_SOURCE_DIR}/examples/*.c")
  file(GLOB_RECURSE TIDY_C_HDRS "${CMAKE_SOURCE_DIR}/src/*.h" "${CMAKE_SOURCE_DIR}/examples/*.h")
  file(GLOB_RECURSE TIDY_CPP "${CMAKE_SOURCE_DIR}/test/*.cpp")
  
  # Need to get the resource dir from the compiler
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} -print-resource-dir
    OUTPUT_VARIABLE CLANG_RESOURCE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  message(STATUS "Clang resource directory: ${CLANG_RESOURCE_DIR}")

  if(TIDY_C_SRC)
    add_custom_target(lint-c-src
      COMMAND ${CLANG_TIDY} -p ${CMAKE_BINARY_DIR} --quiet --extra-arg=-resource-dir=${CLANG_RESOURCE_DIR} ${TIDY_C_SRC}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} VERBATIM)
  endif()

  add_custom_target(lint-c-hdrs
    COMMAND ${CLANG_TIDY} -p ${CMAKE_BINARY_DIR} --quiet --extra-arg-before=-xc --extra-arg=-resource-dir=${CLANG_RESOURCE_DIR} --extra-arg=-I${CMAKE_SOURCE_DIR}/src --header-filter="^${CMAKE_SOURCE_DIR}/src/" ${TIDY_C_HDRS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} VERBATIM)

  if(TIDY_C_SRC)
    add_custom_target(lint-c DEPENDS lint-c-src lint-c-hdrs)
  else()
    add_custom_target(lint-c DEPENDS lint-c-hdrs)
  endif()

  add_test(NAME lint-c COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target lint-c)
  set_tests_properties(lint-c PROPERTIES LABELS "linting")
endif()
