cmake_minimum_required(VERSION 3.30)
project(derivec_project C CXX)

# NOTE: Want latest cpp for easy testing, but C99 for use in C99 codebases
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_C_STANDARD 99)

option(DEVELOP "Enable debug + coverage + sanitizers build" OFF)
option(EXTERNALS "Include targets using externals" ON)

if (DEVELOP)
    # NOTE: not compatible with msan and tsan, but can be easily set by passing CMAKE_{C/CXX}_FLAGS
    message(STATUS "Sanitizers enabled: ASAN & UBSAN")
    set(SANITIZER_FLAGS "-fsanitize=address,undefined")

    message(STATUS "Debug symbols on (including for macros)")
    set(CMAKE_C_FLAGS_DEBUG "-g3")
    set(CMAKE_CXX_FLAGS_DEBUG "-g3")

    message(STATUS "Coverage enabled: GCC/Clang coverage flags")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}  -fprofile-arcs -ftest-coverage -fno-inline -fno-inline-small-functions -fno-inline-functions-called-once")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")

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

find_program(LCOV_EXECUTABLE NAMES lcov)
find_program(GENHTML_EXECUTABLE NAMES genhtml)

if(DEVELOP AND LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
  add_custom_target(coverage
    COMMENT "🔍 Running CTest + lcov → HTML coverage report"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND ${LCOV_EXECUTABLE}
            --directory .
            --zerocounters
    COMMAND ${CMAKE_CTEST_COMMAND}
            --verbose
            --output-on-failure
    COMMAND ${LCOV_EXECUTABLE}
            --directory .
            --capture
            --output-file coverage.info
            --rc geninfo_unexecuted_blocks=1
            --ignore-errors inconsistent # Inconsistencies in the hashmap.cpp file's rc integration
    COMMAND ${LCOV_EXECUTABLE}
            --extract coverage.info 
                "*/src/derive-c/*"
            --output-file coverage.filtered.info
            --ignore-errors unused
    COMMAND ${GENHTML_EXECUTABLE}
            coverage.filtered.info
            --output-directory coverage-report
    VERBATIM
  )
endif()