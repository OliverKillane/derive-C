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
#     Prefix to prepend to all discovered test names. Defaults to "<TARGET>/".
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
#     RUN_COMMAND "{EXE}" "--test={TEST}" "--verbose" "--output-dir=/tmp/results"
#     TEST_PREFIX "unit_tests/"
#     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test_output"
#     TEST_PROPERTIES
#       TIMEOUT 300
#       COST 1.5
#       WILL_FAIL FALSE
#     ENVIRONMENT
#       "TEST_DATA_DIR=/opt/testdata"
#       "LOG_LEVEL=DEBUG"
#       "ENABLE_COVERAGE=ON"
#     LABELS
#       "unit"
#       "fast"
#       "integration"
#   )
#
# This will:
#   1. After building my_test_executable, run: <exe_path> --list-tests
#   2. For each test name discovered, create a CTest entry like:
#      add_test(NAME unit_tests__test_name COMMAND <exe_path> --test=test_name --verbose --output-dir=/tmp/results)
#      set_tests_properties(unit_tests__test_name PROPERTIES
#        WORKING_DIRECTORY "/path/to/test_output"
#        ENVIRONMENT "TEST_DATA_DIR=/opt/testdata;LOG_LEVEL=DEBUG;ENABLE_COVERAGE=ON"
#        LABELS "unit;fast;integration"
#        TIMEOUT 300
#        COST 1.5
#        WILL_FAIL FALSE
#      )
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

  # Infer generated file path
  set(_generated_file "${CMAKE_CURRENT_BINARY_DIR}/${DC_DISCOVER_TESTS_TARGET}_tests.cmake")

  # Set defaults - prefix defaults to target name
  if(NOT DC_DISCOVER_TESTS_TEST_PREFIX)
    set(DC_DISCOVER_TESTS_TEST_PREFIX "${DC_DISCOVER_TESTS_TARGET}/")
  endif()

  if(NOT DC_DISCOVER_TESTS_WORKING_DIRECTORY)
    set(DC_DISCOVER_TESTS_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
  endif()

  # Helper script path
  set(_helper_script "${CMAKE_CURRENT_BINARY_DIR}/discover_tests_${DC_DISCOVER_TESTS_TARGET}.cmake")

  # Create helper script that will be executed post-build
  file(GENERATE
    OUTPUT "${_helper_script}"
    CONTENT
"cmake_minimum_required(VERSION 3.10)

# Read executable path from file (handles generator expressions)
if(DEFINED EXE_FILE AND NOT EXE_FILE STREQUAL \"\")
  # Strip quotes from EXE_FILE path
  string(STRIP \"\${EXE_FILE}\" _exe_file_clean)
  string(REGEX REPLACE \"^\\\"(.*)\\\"\\$\" \"\\\\1\" _exe_file_clean \"\${_exe_file_clean}\")
  get_filename_component(_exe_file_clean \"\${_exe_file_clean}\" ABSOLUTE)
  file(READ \"\${_exe_file_clean}\" EXE)
  string(STRIP \"\${EXE}\" EXE)
  get_filename_component(EXE \"\${EXE}\" ABSOLUTE)
elseif(NOT DEFINED EXE OR EXE STREQUAL \"\")
  message(FATAL_ERROR \"discover_tests helper: EXE not set\")
endif()

# Read output file path
if(NOT DEFINED OUT_CMAKE OR OUT_CMAKE STREQUAL \"\")
  message(FATAL_ERROR \"discover_tests helper: OUT_CMAKE not set\")
endif()
# Strip quotes and normalize
string(STRIP \"\${OUT_CMAKE}\" OUT_CMAKE)
string(REGEX REPLACE \"^\\\"(.*)\\\"\\$\" \"\\\\1\" OUT_CMAKE \"\${OUT_CMAKE}\")
get_filename_component(OUT_CMAKE \"\${OUT_CMAKE}\" ABSOLUTE)

# Read optional parameters
if(NOT DEFINED TEST_PREFIX)
  set(TEST_PREFIX \"\")
endif()
if(NOT DEFINED WORKING_DIRECTORY)
  set(WORKING_DIRECTORY \"\")
else()
  # Strip quotes and normalize
  string(STRIP \"\${WORKING_DIRECTORY}\" WORKING_DIRECTORY)
  string(REGEX REPLACE \"^\\\"(.*)\\\"\\$\" \"\\\\1\" WORKING_DIRECTORY \"\${WORKING_DIRECTORY}\")
  get_filename_component(WORKING_DIRECTORY \"\${WORKING_DIRECTORY}\" ABSOLUTE)
endif()
if(NOT DEFINED ENVIRONMENT)
  set(ENVIRONMENT \"\")
else()
  # Convert semicolon-separated string back to list
  string(REPLACE \"\\\\;\" \";\" ENVIRONMENT \"\${ENVIRONMENT}\")
endif()
if(NOT DEFINED LABELS)
  set(LABELS \"\")
else()
  # Convert semicolon-separated string back to list
  string(REPLACE \"\\\\;\" \";\" LABELS \"\${LABELS}\")
endif()

# Read LIST_COMMAND and RUN_COMMAND from files (strip quotes from paths)
string(STRIP \"\${LIST_COMMAND_FILE}\" _list_cmd_file_clean)
string(REGEX REPLACE \"^\\\"(.*)\\\"\\$\" \"\\\\1\" _list_cmd_file_clean \"\${_list_cmd_file_clean}\")
get_filename_component(_list_cmd_file_clean \"\${_list_cmd_file_clean}\" ABSOLUTE)
file(READ \"\${_list_cmd_file_clean}\" _list_cmd_content)
string(STRIP \"\${_list_cmd_content}\" _list_cmd_content)
string(REPLACE \"\\n\" \";\" _list_cmd_list \"\${_list_cmd_content}\")
list(FILTER _list_cmd_list EXCLUDE REGEX \"^\\$\")

string(STRIP \"\${RUN_COMMAND_FILE}\" _run_cmd_file_clean)
string(REGEX REPLACE \"^\\\"(.*)\\\"\\$\" \"\\\\1\" _run_cmd_file_clean \"\${_run_cmd_file_clean}\")
get_filename_component(_run_cmd_file_clean \"\${_run_cmd_file_clean}\" ABSOLUTE)
file(READ \"\${_run_cmd_file_clean}\" _run_cmd_content)
string(STRIP \"\${_run_cmd_content}\" _run_cmd_content)
string(REPLACE \"\\n\" \";\" _run_cmd_list \"\${_run_cmd_content}\")
list(FILTER _run_cmd_list EXCLUDE REGEX \"^\\$\")

# Replace {EXE} in LIST_COMMAND
set(_list_cmd \"\")
foreach(_a IN LISTS _list_cmd_list)
  string(STRIP \"\${_a}\" _a)
  if(_a STREQUAL \"\")
    continue()
  endif()
  string(REPLACE \"{EXE}\" \"\${EXE}\" _a \"\${_a}\")
  list(APPEND _list_cmd \"\${_a}\")
endforeach()

# Execute list command
list(GET _list_cmd 0 _cmd_executable)
list(SUBLIST _list_cmd 1 -1 _cmd_args)

# Use working directory if specified
if(WORKING_DIRECTORY STREQUAL \"\")
  get_filename_component(_wd \"\${_cmd_executable}\" DIRECTORY)
else()
  set(_wd \"\${WORKING_DIRECTORY}\")
endif()

execute_process(
  COMMAND \${_list_cmd}
  RESULT_VARIABLE _rc
  OUTPUT_VARIABLE _out
  ERROR_VARIABLE _err
  WORKING_DIRECTORY \"\${_wd}\"
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE
)

if(NOT _rc EQUAL 0)
  message(FATAL_ERROR
    \"discover_tests: list command failed (rc=\${_rc})\\n\"
    \"EXE: \${EXE}\\n\"
    \"stderr:\\n\${_err}\\n\"
    \"stdout:\\n\${_out}\\n\")
endif()

# Ensure output directory exists
get_filename_component(_out_dir \"\${OUT_CMAKE}\" DIRECTORY)
if(_out_dir AND NOT EXISTS \"\${_out_dir}\")
  file(MAKE_DIRECTORY \"\${_out_dir}\")
endif()

# Write header
file(WRITE \"\${OUT_CMAKE}\" \"# Generated by discover_tests for \${EXE}\\n\")

if(_out STREQUAL \"\")
  file(APPEND \"\${OUT_CMAKE}\" \"# No tests discovered.\\n\")
  return()
endif()

# Process each test
string(REPLACE \"\\r\\n\" \"\\n\" _out \"\${_out}\")
string(REPLACE \"\\r\" \"\\n\" _out \"\${_out}\")
string(REGEX REPLACE \"\\n+\$\" \"\" _out \"\${_out}\")
string(REPLACE \"\\n\" \";\" _lines \"\${_out}\")

# Track test names to ensure uniqueness
set(_test_counter 0)
set(_seen_names \"\")

foreach(_t IN LISTS _lines)
  string(STRIP \"\${_t}\" _t)
  if(_t STREQUAL \"\")
    continue()
  endif()

  set(_test_name \"\${TEST_PREFIX}\${_t}\")

  # Sanitize CTest name
  string(STRIP \"\${_test_name}\" _ctest_name)
  string(REGEX REPLACE \"^[\\\"']+(.*)[\\\"']+\$\" \"\\\\1\" _ctest_name \"\${_ctest_name}\")
  string(REPLACE \"/\" \"__\" _ctest_name \"\${_ctest_name}\")
  string(REPLACE \" \" \"_\" _ctest_name \"\${_ctest_name}\")
  string(REPLACE \":\" \"_\" _ctest_name \"\${_ctest_name}\")
  string(REPLACE \"<\" \"_\" _ctest_name \"\${_ctest_name}\")
  string(REPLACE \">\" \"_\" _ctest_name \"\${_ctest_name}\")
  string(REPLACE \",\" \"_\" _ctest_name \"\${_ctest_name}\")
  string(REPLACE \"\\\"\" \"\" _ctest_name \"\${_ctest_name}\")
  string(REPLACE \"'\" \"\" _ctest_name \"\${_ctest_name}\")
  
  # Ensure uniqueness by appending counter if needed
  set(_original_ctest_name \"\${_ctest_name}\")
  while(\"\${_ctest_name}\" IN_LIST _seen_names)
    math(EXPR _test_counter \"\${_test_counter} + 1\")
    set(_ctest_name \"\${_original_ctest_name}__\${_test_counter}\")
  endwhile()
  list(APPEND _seen_names \"\${_ctest_name}\")

  # Regex-escape for {TEST_REGEX} placeholder
  string(REGEX REPLACE \"([][.^\\$*+?()|{}\\\\\\\\])\" \"\\\\\\\\\\\\1\" _t_regex \"\${_t}\")

  # Build run command by substituting placeholders
  set(_run_cmd \"\")
  foreach(_a IN LISTS _run_cmd_list)
    string(STRIP \"\${_a}\" _a)
    if(_a STREQUAL \"\")
      continue()
    endif()
    string(REPLACE \"{EXE}\" \"\${EXE}\" _a \"\${_a}\")
    string(REPLACE \"{TEST}\" \"\${_t}\" _a \"\${_a}\")
    string(REPLACE \"{TEST_REGEX}\" \"\${_t_regex}\" _a \"\${_a}\")
    list(APPEND _run_cmd \"\${_a}\")
  endforeach()

  # Write add_test call with proper COMMAND syntax
  file(APPEND \"\${OUT_CMAKE}\" \"add_test(NAME \${_ctest_name} COMMAND\")
  foreach(_a IN LISTS _run_cmd)
    # Escape quotes in the argument
    string(REPLACE \"\\\"\" \"\\\\\\\"\" _a_escaped \"\${_a}\")
    file(APPEND \"\${OUT_CMAKE}\" \" \\\"\${_a_escaped}\\\"\")
  endforeach()
  file(APPEND \"\${OUT_CMAKE}\" \")\\n\")

  # Set test properties if needed
  set(_has_properties FALSE)
  if(NOT WORKING_DIRECTORY STREQUAL \"\")
    set(_has_properties TRUE)
  endif()
  if(DEFINED TEST_PROPERTIES)
    string(STRIP \"\${TEST_PROPERTIES}\" _test_props_stripped)
    if(NOT _test_props_stripped STREQUAL \"\")
      list(LENGTH TEST_PROPERTIES _test_props_len)
      if(_test_props_len GREATER 0)
        set(_has_properties TRUE)
      endif()
    endif()
  endif()
  if(DEFINED ENVIRONMENT AND NOT ENVIRONMENT STREQUAL \"\")
    list(LENGTH ENVIRONMENT _env_len)
    if(_env_len GREATER 0)
      set(_has_properties TRUE)
    endif()
  endif()
  if(DEFINED LABELS AND NOT LABELS STREQUAL \"\")
    list(LENGTH LABELS _labels_len)
    if(_labels_len GREATER 0)
      set(_has_properties TRUE)
    endif()
  endif()

  if(_has_properties)
    file(APPEND \"\${OUT_CMAKE}\" \"set_tests_properties(\${_ctest_name} PROPERTIES\")
    if(NOT WORKING_DIRECTORY STREQUAL \"\")
      file(APPEND \"\${OUT_CMAKE}\" \" WORKING_DIRECTORY \\\"\${WORKING_DIRECTORY}\\\"\")
    endif()
    if(DEFINED ENVIRONMENT AND NOT ENVIRONMENT STREQUAL \"\")
      foreach(_env IN LISTS ENVIRONMENT)
        string(STRIP \"\${_env}\" _env_stripped)
        if(NOT _env_stripped STREQUAL \"\")
          file(APPEND \"\${OUT_CMAKE}\" \" ENVIRONMENT \\\"\${_env_stripped}\\\"\")
        endif()
      endforeach()
    endif()
    if(DEFINED LABELS AND NOT LABELS STREQUAL \"\")
      set(_labels_str \"\")
      foreach(_label IN LISTS LABELS)
        string(STRIP \"\${_label}\" _label_stripped)
        if(NOT _label_stripped STREQUAL \"\")
          if(_labels_str STREQUAL \"\")
            set(_labels_str \"\${_label_stripped}\")
          else()
            set(_labels_str \"\${_labels_str};\${_label_stripped}\")
          endif()
        endif()
      endforeach()
      if(NOT _labels_str STREQUAL \"\")
        file(APPEND \"\${OUT_CMAKE}\" \" LABELS \\\"\${_labels_str}\\\"\")
      endif()
    endif()
    if(DEFINED TEST_PROPERTIES)
      foreach(_p IN LISTS TEST_PROPERTIES)
        string(STRIP \"\${_p}\" _p_stripped)
        if(NOT _p_stripped STREQUAL \"\")
          file(APPEND \"\${OUT_CMAKE}\" \" \\\"\${_p_stripped}\\\"\")
        endif()
      endforeach()
    endif()
    file(APPEND \"\${OUT_CMAKE}\" \")\\n\")
  endif()
endforeach()
"
  )

  # Files to store command templates and executable path
  set(_exe_file "${CMAKE_CURRENT_BINARY_DIR}/discover_tests_${DC_DISCOVER_TESTS_TARGET}_exe.txt")
  set(_list_cmd_file "${CMAKE_CURRENT_BINARY_DIR}/discover_tests_${DC_DISCOVER_TESTS_TARGET}_list_cmd.txt")
  set(_run_cmd_file "${CMAKE_CURRENT_BINARY_DIR}/discover_tests_${DC_DISCOVER_TESTS_TARGET}_run_cmd.txt")

  # Write command templates to files (one element per line)
  # Convert CMake list to newline-separated string
  string(REPLACE ";" "\n" _list_cmd_content "${DC_DISCOVER_TESTS_LIST_COMMAND}")
  file(GENERATE
    OUTPUT "${_list_cmd_file}"
    CONTENT "${_list_cmd_content}\n"
  )

  string(REPLACE ";" "\n" _run_cmd_content "${DC_DISCOVER_TESTS_RUN_COMMAND}")
  file(GENERATE
    OUTPUT "${_run_cmd_file}"
    CONTENT "${_run_cmd_content}\n"
  )

  # Handle TEST_PROPERTIES as a list
  set(_test_props_args "")
  if(DC_DISCOVER_TESTS_TEST_PROPERTIES)
    foreach(_prop IN LISTS DC_DISCOVER_TESTS_TEST_PROPERTIES)
      list(APPEND _test_props_args "-DTEST_PROPERTIES=${_prop}")
    endforeach()
  endif()

  # Handle ENVIRONMENT as a list - pass as semicolon-separated string
  set(_env_arg "")
  if(DC_DISCOVER_TESTS_ENVIRONMENT)
    string(REPLACE ";" "\\;" _env_list "${DC_DISCOVER_TESTS_ENVIRONMENT}")
    set(_env_arg "-DENVIRONMENT=${_env_list}")
  endif()

  # Handle LABELS as a list - pass as semicolon-separated string
  set(_labels_arg "")
  if(DC_DISCOVER_TESTS_LABELS)
    string(REPLACE ";" "\\;" _labels_list "${DC_DISCOVER_TESTS_LABELS}")
    set(_labels_arg "-DLABELS=${_labels_list}")
  endif()

  # Set up POST_BUILD command to discover tests
  # Write executable path to file first (handles generator expressions)
  add_custom_command(
    TARGET ${DC_DISCOVER_TESTS_TARGET}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:${DC_DISCOVER_TESTS_TARGET}>" > "${_exe_file}"
    COMMAND ${CMAKE_COMMAND}
      -DEXE_FILE="${_exe_file}"
      -DOUT_CMAKE="${_generated_file}"
      -DTEST_PREFIX="${DC_DISCOVER_TESTS_TEST_PREFIX}"
      -DWORKING_DIRECTORY="${DC_DISCOVER_TESTS_WORKING_DIRECTORY}"
      -DLIST_COMMAND_FILE="${_list_cmd_file}"
      -DRUN_COMMAND_FILE="${_run_cmd_file}"
      ${_test_props_args}
      ${_env_arg}
      ${_labels_arg}
      -P "${_helper_script}"
    COMMENT "Discovering tests for ${DC_DISCOVER_TESTS_TARGET}"
    VERBATIM
  )

  # Create placeholder file only if it doesn't exist
  # POST_BUILD will overwrite it with actual test discoveries
  if(NOT EXISTS "${_generated_file}")
    file(WRITE "${_generated_file}" "# Test discovery file for ${DC_DISCOVER_TESTS_TARGET}\n# Will be populated after first build\n")
  endif()
  
  # Include the generated file only if it has actual test content
  # Check if file contains add_test (not just placeholder comment)
  if(EXISTS "${_generated_file}")
    file(READ "${_generated_file}" _file_content)
    if(_file_content MATCHES "add_test")
      include("${_generated_file}")
    endif()
  endif()
endfunction()
