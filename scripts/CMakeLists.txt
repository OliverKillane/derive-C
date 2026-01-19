if(LINTING)
    find_program(UV_EXECUTABLE uv REQUIRED)

    add_test(
        NAME lint/derive-c-custom-lints
        COMMAND ${UV_EXECUTABLE} run linting ${CMAKE_CURRENT_SOURCE_DIR}/../src/derive-c
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

    # Notebook linting/formatting with nbqa
    add_test(
        NAME lint/notebooks-format
        COMMAND ${UV_EXECUTABLE} run nbqa ruff format --check notebooks/benchmark/
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_test(
        NAME lint/notebooks-lint
        COMMAND ${UV_EXECUTABLE} run nbqa ruff check notebooks/benchmark/
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_test(
        NAME lint/notebooks-mypy
        COMMAND ${UV_EXECUTABLE} run nbqa mypy notebooks/benchmark/
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_test(
        NAME lint/notebooks-cleared
        COMMAND ${UV_EXECUTABLE} run jupyter nbconvert --clear-output --inplace --to notebook notebooks/benchmark/*.ipynb --stdout
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    set_tests_properties(lint/python-tests PROPERTIES LABELS "linting")
    set_tests_properties(lint/python-mypy PROPERTIES LABELS "linting")
    set_tests_properties(lint/python-format PROPERTIES LABELS "linting")
    set_tests_properties(lint/python-lint PROPERTIES LABELS "linting")
    set_tests_properties(lint/notebooks-format PROPERTIES LABELS "linting")
    set_tests_properties(lint/notebooks-lint PROPERTIES LABELS "linting")
    set_tests_properties(lint/notebooks-mypy PROPERTIES LABELS "linting")
    set_tests_properties(lint/notebooks-cleared PROPERTIES LABELS "linting")
endif()