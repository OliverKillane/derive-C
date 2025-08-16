cmake_minimum_required(VERSION 3.30)
project(derivec_project C CXX)

# Export compilation commands for clang-tidy and other tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# NOTE: Want latest cpp for testing/benchmark, but the derive-c library to be usable for C99 codebases
# set (CMAKE_CXX_STANDARD 23)
# set (CMAKE_C_STANDARD 99)

set( CMAKE_C_STANDARD            99           CACHE STRING "C standard"        FORCE )
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

    if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "DEVELOP flag set: Forcing CMAKE_BUILD_TYPE=Debug")
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    endif()

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

    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT} -i ${SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Reformatting all source files"
    )

    add_custom_target(
        format-check
        COMMAND ${CLANG_FORMAT}
                --dry-run
                --Werror
                ${SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Verifying clang-format compliance (fails if any file needs reformatting)"
    )
endif()

find_program(CLANG_TIDY NAMES clang-tidy)

if(CLANG_TIDY)
  # C: examples/ sources only (since src/ is header-only)
  file(GLOB_RECURSE TIDY_C_SRC  "${CMAKE_SOURCE_DIR}/examples/*.c")
  # Headers: both src/ and examples/
  file(GLOB_RECURSE TIDY_C_HDRS "${CMAKE_SOURCE_DIR}/src/*.h" "${CMAKE_SOURCE_DIR}/examples/*.h")

  # C++: test/
  file(GLOB_RECURSE TIDY_CPP "${CMAKE_SOURCE_DIR}/test/*.cpp")

  # Create separate targets for sources and headers
  if(TIDY_C_SRC)
    add_custom_target(lint-c-src
      COMMAND ${CLANG_TIDY} -p ${CMAKE_BINARY_DIR} --quiet --extra-arg-before=-xc ${TIDY_C_SRC}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} VERBATIM)
  endif()

  add_custom_target(lint-c-hdrs
    COMMAND ${CLANG_TIDY} -p ${CMAKE_BINARY_DIR} --quiet --extra-arg-before=-xc --header-filter="^${CMAKE_SOURCE_DIR}/src/" ${TIDY_C_HDRS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} VERBATIM)

  if(TIDY_C_SRC)
    add_custom_target(lint-c DEPENDS lint-c-src lint-c-hdrs)
  else()
    add_custom_target(lint-c DEPENDS lint-c-hdrs)
  endif()

  add_custom_target(lint-cpp
    COMMAND ${CLANG_TIDY} -p ${CMAKE_BINARY_DIR} --quiet --header-filter="^${CMAKE_SOURCE_DIR}/test/" ${TIDY_CPP}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} VERBATIM)

  add_custom_target(lint DEPENDS lint-c lint-cpp)
endif()
# if(CLANG_TIDY)
#   file(GLOB_RECURSE ALL_SRC
#     "${CMAKE_SOURCE_DIR}/src/*.cpp"
#     "${CMAKE_SOURCE_DIR}/test/*.cpp"
#     "${CMAKE_SOURCE_DIR}/examples/*.cpp"
#   )

#   add_custom_target(lint
#     COMMAND ${CLANG_TIDY}
#             -p ${CMAKE_BINARY_DIR}
#             --header-filter "^${CMAKE_SOURCE_DIR}/(src|test|examples)/"
#             # Suppress diagnostics for all deps
#             --extra-arg-before=--system-header-prefix=${CMAKE_BINARY_DIR}/_deps/
#              --extra-arg-before=-isystem${CMAKE_BINARY_DIR}/_deps/rapidcheck-src/include
#             ${ALL_SRC}
#     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#     COMMENT "Running clang-tidy on project sources"
#     VERBATIM
#   )
# endif()
