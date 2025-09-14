file(GLOB_RECURSE EXAMPLES CONFIGURE_DEPENDS *.c)

function(add_example file)
    file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}" "${file}")
    string(REPLACE "/" "-" example_name "${rel_path}")
    string(REPLACE ".c" "" example_name "${example_name}")

    add_executable(${example_name} "${file}")
    set_property(TARGET ${example_name} PROPERTY LINKER_LANGUAGE C)
    set_property(TARGET ${example_name} PROPERTY C_STANDARD 23)
    target_link_libraries(${example_name} PRIVATE derivec)

    add_test(NAME example-${example_name} COMMAND $<TARGET_FILE:${example_name}>)
    set_tests_properties(example-${example_name} PROPERTIES DEPENDS ${example_name} LABELS "examples")
endfunction()

foreach(EXAMPLE_FILE ${EXAMPLES})
    add_example(${EXAMPLE_FILE})
endforeach()
