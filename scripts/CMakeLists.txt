find_program(UV_EXECUTABLE uv REQUIRED)

add_test(
    NAME lint/derive-c-custom-lints
    COMMAND ${UV_EXECUTABLE} run linting ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(lint/derive-c-custom-lints PROPERTIES LABELS "linting")

add_test(
    NAME lint/python-tests
    COMMAND ${UV_EXECUTABLE} run pytest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME lint/python-mypy
    COMMAND ${UV_EXECUTABLE} run mypy .
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME lint/python-format
    COMMAND ${UV_EXECUTABLE} run ruff format --check
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME lint/python-lint
    COMMAND ${UV_EXECUTABLE} run ruff check
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set_tests_properties(lint/python-tests PROPERTIES LABELS "linting")
