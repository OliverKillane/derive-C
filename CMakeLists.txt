cmake_minimum_required(VERSION 3.30)
project(derivec_project C CXX)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(examples)
add_subdirectory(bench)

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