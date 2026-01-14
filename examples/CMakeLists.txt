# Examples

# Optional toggle for dumping IR (very noisy)
option(CLANG_IR_DUMPS "Dump IR before/after all passes" OFF)

function(enable_clang_opt_reports target)
  target_compile_options(${target} PRIVATE
    $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-gline-tables-only>
    $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Rpass=inline>
    $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Rpass-missed=inline>
    $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Rpass-analysis=inline>
    $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Rpass=loop-vectorize>
    $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Rpass-missed=loop-vectorize>
    $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Rpass-analysis=loop-vectorize>
    $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Rpass=slp-vectorizer>
    $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Rpass-missed=slp-vectorizer>
    $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Rpass-analysis=slp-vectorizer>
    $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-fsave-optimization-record>
    $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Xclang>
    $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-fdebug-pass-manager>
  )
endfunction()

function(add_example file)
  file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}" "${file}")
  get_filename_component(example_name_we "${rel_path}" NAME_WE)
  get_filename_component(example_dir "${rel_path}" DIRECTORY)
  set(example_target "${example_dir}/${example_name_we}")
  string(REPLACE "/" "_" example_target_safe "${example_target}")

  add_executable(${example_target_safe} ${file})

  target_link_libraries(${example_target_safe} PRIVATE derivec dc_warnings_c)

  if(OPT_REPORTS)
    enable_clang_opt_reports(${example_target_safe})
  endif()

  set_target_properties(${example_target_safe} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${example_dir}"
    OUTPUT_NAME "${example_name_we}"
  )
  set_property(TARGET ${example_target_safe} PROPERTY LINKER_LANGUAGE C)
  set_property(TARGET ${example_target_safe} PROPERTY C_STANDARD 23)
  target_link_libraries(${example_target_safe} PRIVATE derivec)

  set_property(GLOBAL APPEND PROPERTY EXAMPLE_TARGETS ${example_target_safe})

  add_test(NAME examples/${example_target} COMMAND $<TARGET_FILE:${example_target_safe}>)
  set_tests_properties(examples/${example_target}
    PROPERTIES
      DEPENDS ${example_target_safe}
      LABELS "examples"
  )
endfunction()

file(GLOB_RECURSE EXAMPLES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

foreach(EXAMPLE_FILE ${EXAMPLES})
  add_example(${EXAMPLE_FILE})
endforeach()

if(OPT_REPORTS)
  # (optional) ensure the tool exists
  find_program(LLVM_OPT_REPORT llvm-opt-report REQUIRED)

  # fetch the list of example targets
  get_property(_examples GLOBAL PROPERTY EXAMPLE_TARGETS)

  file(GLOB_RECURSE OPT_YAMLS "${CMAKE_BINARY_DIR}/*.opt.yaml")

  add_custom_target(opt-report
    # Build all examples first, so *.opt.yaml exist:
    DEPENDS ${_examples}
    COMMAND ${LLVM_OPT_REPORT}
            -o "${CMAKE_BINARY_DIR}/opt-report"
            --source-dir "${CMAKE_SOURCE_DIR}"
            ${OPT_YAMLS}
    BYPRODUCTS "${CMAKE_BINARY_DIR}/opt-report/index.html"
    COMMENT "Generating HTML optimization report"
    VERBATIM
  )
endif()
