include(FetchContent)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(COVERAGE_FLAGS
      -fprofile-instr-generate
      -fcoverage-mapping
    )
endif()

# Rapidcheck contains cpp23 deprecated features
add_compile_options(-Wno-deprecated-declarations)
add_compile_options(${COVERAGE_FLAGS})
add_link_options(${COVERAGE_FLAGS})

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

set(RC_ENABLE_GTEST ON CACHE BOOL "Rapidcheck GTest Support" FORCE)
FetchContent_Declare(
    rapidcheck
    GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
    GIT_TAG ff6af6fc683159deb51c543b065eba14dfcf329b # Latest as of 3/30/2025
)
FetchContent_MakeAvailable(rapidcheck)

include(GoogleTest)

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS *.cpp *.hpp)
add_executable(derivec_tests ${TEST_SOURCES})
target_link_libraries(derivec_tests
    PRIVATE
        derivec
        gtest
        gtest_main
        rapidcheck
        rapidcheck_gtest
)

gtest_discover_tests(derivec_tests
    DISCOVERY_TIMEOUT 30
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    PROPERTIES
    ENVIRONMENT
      "RC_PARAMS=max_success=10000"
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    find_program(LLVM_COV_EXECUTABLE NAMES llvm-cov)
    find_program(LLVM_PROFDATA_EXECUTABLE NAMES llvm-profdata)
    find_program(GENHTML_EXECUTABLE NAMES genhtml)
    if(DEVELOP AND LLVM_COV_EXECUTABLE AND LLVM_PROFDATA_EXECUTABLE AND GENHTML_EXECUTABLE)    
        add_custom_target(coverage
            DEPENDS derivec_tests
            COMMENT "🔍 Running llvm-cov → HTML coverage report (Clang/LLVM)"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND LLVM_PROFILE_FILE=default.profraw ${CMAKE_BINARY_DIR}/test/derivec_tests
            COMMAND pwd
            COMMAND ${LLVM_PROFDATA_EXECUTABLE} merge -sparse default.profraw -o coverage.profdata
            COMMAND ${LLVM_COV_EXECUTABLE} show
                    $<TARGET_FILE:derivec_tests>
                    -instr-profile=coverage.profdata
                    -format=html
                    -output-dir=coverage_html
                    ${CMAKE_SOURCE_DIR}/src/derive-c
            COMMENT "Running tests and generating coverage report (clang)"
            VERBATIM
        )
    endif()
endif()
