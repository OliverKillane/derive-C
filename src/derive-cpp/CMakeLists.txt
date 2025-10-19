file(GLOB SOURCES *.hpp)

add_library(derivecpp INTERFACE ${SOURCES})
set_property(TARGET derivecpp PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET derivecpp PROPERTY CXX_STANDARD 23)

target_include_directories(derivecpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
