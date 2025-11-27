function(add_derive_linter_check TARGET_NAME SOURCE_DIR)
    find_program(UV_EXECUTABLE uv REQUIRED)
    
    # Use 'uv run lints' which corresponds to the script defined in pyproject.toml
    add_test(
        NAME ${TARGET_NAME}
        COMMAND ${UV_EXECUTABLE} run lints ${SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    set_tests_properties(${TARGET_NAME} PROPERTIES LABELS "linting")
endfunction()

find_program(UV_EXECUTABLE uv REQUIRED)
add_test(
    NAME lint/python-tests
    COMMAND ${UV_EXECUTABLE} run pytest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(lint/python-tests PROPERTIES LABELS "linting")
