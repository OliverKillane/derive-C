cmake_minimum_required(VERSION 3.30)
project(derivec_project C CXX)

# NOTE: Want latest cpp for easy testing, but C99 for use in C99 codebases
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_C_STANDARD 99)

option(DEBUG "Enable basic debug build" OFF)
option(EXTERNALS "Include targets using externals" ON)

if (DEBUG)
    # NOTE: not compatible with msan and tsan, but can be easily set by passing CMAKe_C/CXX_FLAGS
    message(STATUS "Sanitizers enabled: ASAN & UBSAN")
    set(SANITIZER_FLAGS "-fsanitize=address,undefined")

    if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "DEBUG flag set: Forcing CMAKE_BUILD_TYPE=Debug")
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
endif()

# TODO: Also format CMakeLists.txt files
find_program(CLANG_FORMAT NAMES clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/*.c
        ${CMAKE_SOURCE_DIR}/*.h
        ${CMAKE_SOURCE_DIR}/*.cpp
    )
    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT} -i ${SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()