cmake_minimum_required(VERSION 3.30)
project(derivec_project C CXX)

# NOTE: Want latest cpp for testing/benchmark, but the derive-c library to be usable for C99 codebases
# set (CMAKE_CXX_STANDARD 23)
# set (CMAKE_C_STANDARD 99)

set( CMAKE_C_STANDARD            99           CACHE STRING "C standard"        FORCE )
set( CMAKE_C_STANDARD_REQUIRED   ON           CACHE BOOL   "Require C std"     FORCE )
set( CMAKE_C_EXTENSIONS          OFF          CACHE BOOL   "No compiler extensions" FORCE )

set( CMAKE_CXX_STANDARD          23           CACHE STRING "C++ standard"      FORCE )
set( CMAKE_CXX_STANDARD_REQUIRED ON           CACHE BOOL   "Require C++ std"   FORCE )
set( CMAKE_CXX_EXTENSIONS        OFF          CACHE BOOL   "No compiler extensions" FORCE )

option(DEVELOP "Enable debug + coverage + sanitizers build" OFF)
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
