file(GLOB EXAMPLES CONFIGURE_DEPENDS *.c)

# foreach(EXAMPLE_FILE ${EXAMPLES})
#     get_filename_component(EXAMPLE_NAME ${EXAMPLE_FILE} NAME_WE)
#     add_executable(${EXAMPLE_NAME} ${EXAMPLE_FILE})
#     set_property(TARGET ${EXAMPLE_NAME} PROPERTY LINKER_LANGUAGE C)
#     set_property(TARGET ${EXAMPLE_NAME} PROPERTY C_STANDARD 23)
#     target_link_libraries(${EXAMPLE_NAME} PRIVATE derivec)
# endforeach()

function(add_example name)
    set(example_file "${CMAKE_CURRENT_SOURCE_DIR}/${name}.c")
    get_filename_component(example_name ${example_file} NAME_WE)

    add_executable(${example_name} ${example_file})
    set_property(TARGET ${example_name} PROPERTY LINKER_LANGUAGE C)
    set_property(TARGET ${example_name} PROPERTY C_STANDARD 23)
    target_link_libraries(${example_name} PRIVATE derivec)
endfunction()

add_example(prime_sieve)
add_example(maps)
add_example(derives)
add_example(employees)

# Additional tests for some examples
add_test(NAME PrimeSieveExampleTest COMMAND $<TARGET_FILE:prime_sieve> 23)
set_tests_properties(PrimeSieveExampleTest PROPERTIES DEPENDS prime_sieve)

add_test(NAME MapsExampleTest COMMAND $<TARGET_FILE:maps>)
set_tests_properties(MapsExampleTest PROPERTIES DEPENDS maps)

add_test(NAME DerivesExampleTest COMMAND $<TARGET_FILE:derives>)
set_tests_properties(DerivesExampleTest PROPERTIES DEPENDS derives)

add_test(NAME EmployeesExampleTest COMMAND $<TARGET_FILE:employees>)
set_tests_properties(EmployeesExampleTest PROPERTIES DEPENDS employees)
