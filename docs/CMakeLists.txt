include(FetchContent)

find_package(Doxygen)

FetchContent_Declare(
    doxygen-awesome
    GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
    GIT_TAG v2.3.4
)
FetchContent_MakeAvailable(doxygen-awesome)
FetchContent_GetProperties(doxygen-awesome SOURCE_DIR AWESOME_CSS_DIR)

set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/../docs/Doxyfile.in)
set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

file(GLOB_RECURSE DOC_INPUT_FILES
    ${CMAKE_SOURCE_DIR}/src/*
    ${CMAKE_SOURCE_DIR}/examples/*
    ${CMAKE_SOURCE_DIR}/README.md
)
list(FILTER DOC_INPUT_FILES EXCLUDE REGEX ".*/_deps/.*")
list(JOIN DOC_INPUT_FILES " " DOXYGEN_INPUT_FILES)

configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/html/index.html
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    DEPENDS ${DOXYGEN_OUT} ${DOC_INPUT_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating documentation with Doxygen"
)

add_custom_target(docs ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/html/index.html
)
