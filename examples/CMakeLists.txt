file(GLOB EXAMPLES *.c)

foreach(EXAMPLE_FILE ${EXAMPLES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_FILE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_FILE})
    set_property(TARGET ${EXAMPLE_NAME} PROPERTY LINKER_LANGUAGE C)
    set_property(TARGET ${EXAMPLE_NAME} PROPERTY C_STANDARD 23)
    target_link_libraries(${EXAMPLE_NAME} PRIVATE derivec)
endforeach()
