function(add_example file)
  file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}" "${file}")
  get_filename_component(example_name_we "${rel_path}" NAME_WE)
  get_filename_component(example_dir "${rel_path}" DIRECTORY)
  set(example_target "${example_dir}/${example_name_we}")
  string(REPLACE "/" "_" example_target_safe "${example_target}")

  add_executable(${example_target_safe} ${file})
  set_target_properties(${example_target_safe} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${example_dir}"
    OUTPUT_NAME "${example_name_we}"
  )
  set_property(TARGET ${example_target_safe} PROPERTY LINKER_LANGUAGE C)
  set_property(TARGET ${example_target_safe} PROPERTY C_STANDARD 23)
  target_link_libraries(${example_target_safe} PRIVATE derivec)

  add_test(NAME examples/${example_target} COMMAND $<TARGET_FILE:${example_target_safe}>)
  set_tests_properties(examples/${example_target} 
    PROPERTIES
      DEPENDS ${example_target_safe} 
      LABELS "examples"
  )
endfunction()

file(GLOB_RECURSE EXAMPLES CONFIGURE_DEPENDS *.c)

foreach(EXAMPLE_FILE ${EXAMPLES})
  add_example(${EXAMPLE_FILE})
endforeach()
