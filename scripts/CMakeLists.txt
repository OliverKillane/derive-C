add_test(
    NAME lint/derive-c-custom-lints
    COMMAND ${UV_EXECUTABLE} run linting ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(lint/derive-c-custom-lints PROPERTIES LABELS "linting")

find_program(UV_EXECUTABLE uv REQUIRED)
add_test(
    NAME lint/python-tests
    COMMAND ${UV_EXECUTABLE} run pytest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(lint/python-tests PROPERTIES LABELS "linting")
