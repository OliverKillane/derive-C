# JUSTIFY: Including cpp headers
#  - For utilities when testing code (e.g. trampoline)
file(GLOB SOURCES *.h *.hpp)

add_library(derivec INTERFACE ${SOURCES})
set_property(TARGET derivec PROPERTY LINKER_LANGUAGE C)
set_property(TARGET derivec PROPERTY C_STANDARD 23)
set_property(TARGET derivec PROPERTY CMAKE_CXX_STANDARD 23)

target_include_directories(derivec INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
