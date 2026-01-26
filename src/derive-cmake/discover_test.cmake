#[[
# Discovers tests dynamically from a test executable and registers them with CTest.
#
# This function runs a LIST_COMMAND to discover available tests from the target executable,
# then generates CTest test entries for each discovered test. Tests are discovered at build
# time via a POST_BUILD command, allowing test discovery to work with generator expressions
# and ensuring the executable exists before discovery runs.
#
# Required Arguments:
#   TARGET <target_name>
#     The CMake target (executable) to discover tests from.
#
#   LIST_COMMAND <command> [<arg>...]
#     Command to list available tests. The executable path can be referenced using the
#     {EXE} placeholder, which will be replaced with the actual executable path at runtime.
#     The command should output one test name per line on stdout.
#
#   RUN_COMMAND <command> [<arg>...]
#     Command template to run a single test. Supports placeholders:
#       {EXE}      - Replaced with the executable path
#       {TEST}     - Replaced with the test name (as listed by LIST_COMMAND)
#       {TEST_REGEX} - Replaced with a regex-escaped version of the test name
#
# Optional Arguments:
#   TEST_PREFIX <prefix>
#     Prefix to prepend to all discovered test names. Defaults to "<TARGET>_".
#     Test names are sanitized for CTest (special characters replaced with underscores).
#
#   WORKING_DIRECTORY <dir>
#     Working directory for both LIST_COMMAND and RUN_COMMAND execution.
#     Defaults to CMAKE_CURRENT_BINARY_DIR.
#
#   TEST_PROPERTIES <prop1> [<prop2>...]
#     Additional test properties to set on each discovered test (passed to set_tests_properties).
#
#   ENVIRONMENT <var1>=<value1> [<var2>=<value2>...]
#     Environment variables to set for test execution.
#
#   LABELS <label1> [<label2>...]
#     Labels to assign to all discovered tests (useful for filtering with ctest -L).
#
# Example Usage:
#   dc_discover_tests(
#     TARGET my_test_executable
#     LIST_COMMAND "{EXE}" "--list-tests"
#     RUN_COMMAND "{EXE}" "--test={TEST}" "--verbose"
#     TEST_PREFIX "unit_tests_"
#     LABELS "unit" "fast"
#   )
#]]
function(dc_discover_tests)
  cmake_parse_arguments(
    PARSE_ARGV 0
    DC_DISCOVER_TESTS
    ""
    "TARGET;TEST_PREFIX;WORKING_DIRECTORY"
    "LIST_COMMAND;RUN_COMMAND;TEST_PROPERTIES;ENVIRONMENT;LABELS"
  )

  if(NOT DC_DISCOVER_TESTS_TARGET)
    message(FATAL_ERROR "dc_discover_tests: TARGET is required")
  endif()

  if(NOT DC_DISCOVER_TESTS_LIST_COMMAND)
    message(FATAL_ERROR "dc_discover_tests: LIST_COMMAND is required")
  endif()

  if(NOT DC_DISCOVER_TESTS_RUN_COMMAND)
    message(FATAL_ERROR "dc_discover_tests: RUN_COMMAND is required")
  endif()

  # Set defaults
  if(NOT DC_DISCOVER_TESTS_TEST_PREFIX)
    set(DC_DISCOVER_TESTS_TEST_PREFIX "${DC_DISCOVER_TESTS_TARGET}_")
  endif()

  if(NOT DC_DISCOVER_TESTS_WORKING_DIRECTORY)
    set(DC_DISCOVER_TESTS_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
  endif()

  # Generated files - following gtest pattern
  get_property(
    has_counter
    TARGET ${DC_DISCOVER_TESTS_TARGET}
    PROPERTY CTEST_DISCOVERED_TEST_COUNTER
    SET
  )
  if(has_counter)
    get_property(
      counter
      TARGET ${DC_DISCOVER_TESTS_TARGET}
      PROPERTY CTEST_DISCOVERED_TEST_COUNTER
    )
    math(EXPR counter "${counter} + 1")
  else()
    set(counter 1)
  endif()
  set_property(
    TARGET ${DC_DISCOVER_TESTS_TARGET}
    PROPERTY CTEST_DISCOVERED_TEST_COUNTER
    ${counter}
  )

  set(ctest_file_base "${CMAKE_CURRENT_BINARY_DIR}/${DC_DISCOVER_TESTS_TARGET}[${counter}]")
  set(ctest_include_file "${ctest_file_base}_include.cmake")
  set(ctest_tests_file "${ctest_file_base}_tests.cmake")
  set(helper_script "${CMAKE_CURRENT_BINARY_DIR}/discover_tests_${DC_DISCOVER_TESTS_TARGET}.cmake")

  # Prepare arguments for passing to helper script
  string(REPLACE ";" "\\;" _list_cmd_escaped "${DC_DISCOVER_TESTS_LIST_COMMAND}")
  string(REPLACE ";" "\\;" _run_cmd_escaped "${DC_DISCOVER_TESTS_RUN_COMMAND}")

  set(_test_props_escaped "")
  if(DC_DISCOVER_TESTS_TEST_PROPERTIES)
    string(REPLACE ";" "\\;" _test_props_escaped "${DC_DISCOVER_TESTS_TEST_PROPERTIES}")
  endif()

  set(_env_escaped "")
  if(DC_DISCOVER_TESTS_ENVIRONMENT)
    string(REPLACE ";" "\\;" _env_escaped "${DC_DISCOVER_TESTS_ENVIRONMENT}")
  endif()

  set(_labels_escaped "")
  if(DC_DISCOVER_TESTS_LABELS)
    string(REPLACE ";" "\\;" _labels_escaped "${DC_DISCOVER_TESTS_LABELS}")
  endif()

  # Create helper script that will be executed POST_BUILD
  file(GENERATE
    OUTPUT "${helper_script}"
    CONTENT
"cmake_minimum_required(VERSION 3.10)

# Required variables
if(NOT DEFINED TEST_EXECUTABLE OR TEST_EXECUTABLE STREQUAL \"\")
  message(FATAL_ERROR \"discover_tests: TEST_EXECUTABLE not set\")
endif()
if(NOT DEFINED CTEST_FILE OR CTEST_FILE STREQUAL \"\")
  message(FATAL_ERROR \"discover_tests: CTEST_FILE not set\")
endif()
if(NOT DEFINED LIST_COMMAND OR LIST_COMMAND STREQUAL \"\")
  message(FATAL_ERROR \"discover_tests: LIST_COMMAND not set\")
endif()
if(NOT DEFINED RUN_COMMAND OR RUN_COMMAND STREQUAL \"\")
  message(FATAL_ERROR \"discover_tests: RUN_COMMAND not set\")
endif()

# Optional variables with defaults
if(NOT DEFINED TEST_PREFIX)
  set(TEST_PREFIX \"\")
endif()
if(NOT DEFINED WORKING_DIRECTORY)
  set(WORKING_DIRECTORY \"\")
endif()
if(NOT DEFINED TEST_PROPERTIES)
  set(TEST_PROPERTIES \"\")
endif()
if(NOT DEFINED ENVIRONMENT)
  set(ENVIRONMENT \"\")
endif()
if(NOT DEFINED LABELS)
  set(LABELS \"\")
endif()

# Unescape semicolons in lists
string(REPLACE \"\\\\;\" \";\" LIST_COMMAND \"\${LIST_COMMAND}\")
string(REPLACE \"\\\\;\" \";\" RUN_COMMAND \"\${RUN_COMMAND}\")
string(REPLACE \"\\\\;\" \";\" TEST_PROPERTIES \"\${TEST_PROPERTIES}\")
string(REPLACE \"\\\\;\" \";\" ENVIRONMENT \"\${ENVIRONMENT}\")
string(REPLACE \"\\\\;\" \";\" LABELS \"\${LABELS}\")

# Replace {EXE} in LIST_COMMAND
set(_list_cmd \"\")
foreach(_arg IN LISTS LIST_COMMAND)
  string(REPLACE \"{EXE}\" \"\${TEST_EXECUTABLE}\" _arg \"\${_arg}\")
  list(APPEND _list_cmd \"\${_arg}\")
endforeach()

# Determine working directory
if(WORKING_DIRECTORY STREQUAL \"\")
  get_filename_component(_wd \"\${TEST_EXECUTABLE}\" DIRECTORY)
else()
  set(_wd \"\${WORKING_DIRECTORY}\")
endif()

# Execute list command to discover tests
execute_process(
  COMMAND \${_list_cmd}
  RESULT_VARIABLE _rc
  OUTPUT_VARIABLE _test_output
  ERROR_VARIABLE _err
  WORKING_DIRECTORY \"\${_wd}\"
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE
)

if(NOT _rc EQUAL 0)
  message(FATAL_ERROR
    \"discover_tests: list command failed (rc=\${_rc})\\n\"
    \"EXECUTABLE: \${TEST_EXECUTABLE}\\n\"
    \"COMMAND: \${_list_cmd}\\n\"
    \"stderr:\\n\${_err}\\n\"
    \"stdout:\\n\${_test_output}\\n\")
endif()

# Ensure output directory exists
get_filename_component(_out_dir \"\${CTEST_FILE}\" DIRECTORY)
if(_out_dir AND NOT EXISTS \"\${_out_dir}\")
  file(MAKE_DIRECTORY \"\${_out_dir}\")
endif()

# Write header
file(WRITE \"\${CTEST_FILE}\" \"# Generated by discover_tests for \${TEST_EXECUTABLE}\\n\")

# Process test output
if(_test_output STREQUAL \"\")
  file(APPEND \"\${CTEST_FILE}\" \"# No tests discovered.\\n\")
  return()
endif()

# Normalize line endings and split into lines
string(REPLACE \"\\r\\n\" \"\\n\" _test_output \"\${_test_output}\")
string(REPLACE \"\\r\" \"\\n\" _test_output \"\${_test_output}\")
string(REGEX REPLACE \"\\n+\$\" \"\" _test_output \"\${_test_output}\")
string(REPLACE \"\\n\" \";\" _test_lines \"\${_test_output}\")

# Track test names for uniqueness
set(_test_counter 0)
set(_seen_names \"\")

# Process each test
foreach(_test_name IN LISTS _test_lines)
  string(STRIP \"\${_test_name}\" _test_name)
  if(_test_name STREQUAL \"\")
    continue()
  endif()

  # Build CTest name with prefix
  set(_ctest_name \"\${TEST_PREFIX}\${_test_name}\")

  # Sanitize CTest name (replace special characters with underscores)
  string(REPLACE \"/\" \"__\" _ctest_name \"\${_ctest_name}\")
  string(REPLACE \" \" \"_\" _ctest_name \"\${_ctest_name}\")
  string(REPLACE \":\" \"_\" _ctest_name \"\${_ctest_name}\")
  string(REPLACE \"<\" \"_\" _ctest_name \"\${_ctest_name}\")
  string(REPLACE \">\" \"_\" _ctest_name \"\${_ctest_name}\")
  string(REPLACE \",\" \"_\" _ctest_name \"\${_ctest_name}\")
  string(REPLACE \"\\\"\" \"\" _ctest_name \"\${_ctest_name}\")
  string(REPLACE \"'\" \"\" _ctest_name \"\${_ctest_name}\")

  # Ensure uniqueness
  set(_original_ctest_name \"\${_ctest_name}\")
  while(\"\${_ctest_name}\" IN_LIST _seen_names)
    math(EXPR _test_counter \"\${_test_counter} + 1\")
    set(_ctest_name \"\${_original_ctest_name}__\${_test_counter}\")
  endwhile()
  list(APPEND _seen_names \"\${_ctest_name}\")

  # Regex-escape test name for {TEST_REGEX} placeholder
  string(REGEX REPLACE \"([][.^\\$*+?()|{}\\\\\\\\])\" \"\\\\\\\\\\\\1\" _test_regex \"\${_test_name}\")

  # Build run command with placeholder substitution
  set(_run_cmd \"\")
  foreach(_arg IN LISTS RUN_COMMAND)
    string(REPLACE \"{EXE}\" \"\${TEST_EXECUTABLE}\" _arg \"\${_arg}\")
    string(REPLACE \"{TEST}\" \"\${_test_name}\" _arg \"\${_arg}\")
    string(REPLACE \"{TEST_REGEX}\" \"\${_test_regex}\" _arg \"\${_arg}\")
    list(APPEND _run_cmd \"\${_arg}\")
  endforeach()

  # Write add_test using old-style syntax: add_test(name executable [arg...])
  # The test name is sanitized so it doesn't need quotes
  # But we quote all arguments to be safe
  file(APPEND \"\${CTEST_FILE}\" \"add_test(\${_ctest_name}\")
  foreach(_arg IN LISTS _run_cmd)
    # Escape for CMake file: escape backslashes and quotes
    string(REPLACE \"\\\\\" \"\\\\\\\\\" _arg_escaped \"\${_arg}\")
    string(REPLACE \"\\\"\" \"\\\\\\\"\" _arg_escaped \"\${_arg_escaped}\")
    file(APPEND \"\${CTEST_FILE}\" \" \\\"\${_arg_escaped}\\\"\")
  endforeach()
  file(APPEND \"\${CTEST_FILE}\" \")\\n\")

  # Set test properties if needed
  set(_has_props FALSE)
  if(NOT WORKING_DIRECTORY STREQUAL \"\")
    set(_has_props TRUE)
  endif()
  if(TEST_PROPERTIES)
    list(LENGTH TEST_PROPERTIES _props_len)
    if(_props_len GREATER 0)
      set(_has_props TRUE)
    endif()
  endif()
  if(ENVIRONMENT)
    list(LENGTH ENVIRONMENT _env_len)
    if(_env_len GREATER 0)
      set(_has_props TRUE)
    endif()
  endif()
  if(LABELS)
    list(LENGTH LABELS _labels_len)
    if(_labels_len GREATER 0)
      set(_has_props TRUE)
    endif()
  endif()

  if(_has_props)
    file(APPEND \"\${CTEST_FILE}\" \"set_tests_properties(\${_ctest_name} PROPERTIES\")
    if(NOT WORKING_DIRECTORY STREQUAL \"\")
      string(REPLACE \"\\\\\" \"\\\\\\\\\" _wd_escaped \"\${WORKING_DIRECTORY}\")
      string(REPLACE \"\\\"\" \"\\\\\\\"\" _wd_escaped \"\${_wd_escaped}\")
      file(APPEND \"\${CTEST_FILE}\" \" WORKING_DIRECTORY \\\"\${_wd_escaped}\\\"\")
    endif()
    if(ENVIRONMENT)
      foreach(_env IN LISTS ENVIRONMENT)
        string(REPLACE \"\\\\\" \"\\\\\\\\\" _env_escaped \"\${_env}\")
        string(REPLACE \"\\\"\" \"\\\\\\\"\" _env_escaped \"\${_env_escaped}\")
        file(APPEND \"\${CTEST_FILE}\" \" ENVIRONMENT \\\"\${_env_escaped}\\\"\")
      endforeach()
    endif()
    if(LABELS)
      set(_labels_str \"\")
      foreach(_label IN LISTS LABELS)
        if(_labels_str STREQUAL \"\")
          set(_labels_str \"\${_label}\")
        else()
          set(_labels_str \"\${_labels_str};\${_label}\")
        endif()
      endforeach()
      string(REPLACE \"\\\\\" \"\\\\\\\\\" _labels_escaped \"\${_labels_str}\")
      string(REPLACE \"\\\"\" \"\\\\\\\"\" _labels_escaped \"\${_labels_escaped}\")
      file(APPEND \"\${CTEST_FILE}\" \" LABELS \\\"\${_labels_escaped}\\\"\")
    endif()
    if(TEST_PROPERTIES)
      foreach(_prop IN LISTS TEST_PROPERTIES)
        string(REPLACE \"\\\\\" \"\\\\\\\\\" _prop_escaped \"\${_prop}\")
        string(REPLACE \"\\\"\" \"\\\\\\\"\" _prop_escaped \"\${_prop_escaped}\")
        file(APPEND \"\${CTEST_FILE}\" \" \\\"\${_prop_escaped}\\\"\")
      endforeach()
    endif()
    file(APPEND \"\${CTEST_FILE}\" \")\\n\")
  endif()
endforeach()
"
  )

  # Set up POST_BUILD command to discover tests (following gtest pattern)
  add_custom_command(
    TARGET ${DC_DISCOVER_TESTS_TARGET}
    POST_BUILD
    BYPRODUCTS "${ctest_tests_file}"
    COMMAND "${CMAKE_COMMAND}"
      -D "TEST_EXECUTABLE=$<TARGET_FILE:${DC_DISCOVER_TESTS_TARGET}>"
      -D "CTEST_FILE=${ctest_tests_file}"
      -D "LIST_COMMAND=${_list_cmd_escaped}"
      -D "RUN_COMMAND=${_run_cmd_escaped}"
      -D "TEST_PREFIX=${DC_DISCOVER_TESTS_TEST_PREFIX}"
      -D "WORKING_DIRECTORY=${DC_DISCOVER_TESTS_WORKING_DIRECTORY}"
      -D "TEST_PROPERTIES=${_test_props_escaped}"
      -D "ENVIRONMENT=${_env_escaped}"
      -D "LABELS=${_labels_escaped}"
      -P "${helper_script}"
    COMMENT "Discovering tests for ${DC_DISCOVER_TESTS_TARGET}"
    VERBATIM
  )

  # Create include wrapper file (following gtest pattern)
  file(WRITE "${ctest_include_file}"
    "if(EXISTS \"${ctest_tests_file}\")\n"
    "  include(\"${ctest_tests_file}\")\n"
    "else()\n"
    "  add_test(${DC_DISCOVER_TESTS_TARGET}_NOT_BUILT ${DC_DISCOVER_TESTS_TARGET}_NOT_BUILT)\n"
    "endif()\n"
  )

  # Add to TEST_INCLUDE_FILES so CMake processes it when ctest runs
  set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${ctest_include_file}")
endfunction()
