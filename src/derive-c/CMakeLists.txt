file(GLOB SOURCES *.h)

add_library(derivec INTERFACE ${SOURCES})
target_compile_definitions(derivec INTERFACE _GNU_SOURCE)
set_property(TARGET derivec PROPERTY LINKER_LANGUAGE C)
set_property(TARGET derivec PROPERTY C_STANDARD 23)

target_include_directories(derivec INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
