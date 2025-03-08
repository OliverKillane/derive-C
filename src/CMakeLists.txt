file(GLOB SOURCES *.h *.template)

add_library(derivec INTERFACE ${SOURCES})
set_property(TARGET derivec PROPERTY LINKER_LANGUAGE C)
target_include_directories(derivec INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
