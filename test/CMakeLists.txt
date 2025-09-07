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

# Find all .cpp test files
file(GLOB_RECURSE TEST_CPP_FILES CONFIGURE_DEPENDS *.cpp)

# Create a list to hold all test executables for coverage
set(ALL_TEST_EXECUTABLES)

# Create separate executable for each .cpp file
foreach(TEST_FILE ${TEST_CPP_FILES})
    # Get relative path and create executable name
    file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_FILE})
    string(REPLACE "/" "_" TEST_NAME ${REL_PATH})
    string(REPLACE ".cpp" "" TEST_NAME ${TEST_NAME})
    
    # Create executable
    add_executable(${TEST_NAME} ${TEST_FILE})
    
    # Link libraries
    target_link_libraries(${TEST_NAME}
        PRIVATE
            derivec
            gtest
            gtest_main
            rapidcheck
            rapidcheck_gtest
    )
    
    # Discover tests for this executable
    gtest_discover_tests(${TEST_NAME}
        DISCOVERY_TIMEOUT 30
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PROPERTIES
        ENVIRONMENT
          "RC_PARAMS=max_success=10000"
    )
    
    # Add to list for coverage
    list(APPEND ALL_TEST_EXECUTABLES ${TEST_NAME})
endforeach()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    find_program(LLVM_COV_EXECUTABLE NAMES llvm-cov)
    find_program(LLVM_PROFDATA_EXECUTABLE NAMES llvm-profdata)
    find_program(GENHTML_EXECUTABLE NAMES genhtml)
    if(DEVELOP AND LLVM_COV_EXECUTABLE AND LLVM_PROFDATA_EXECUTABLE AND GENHTML_EXECUTABLE)    
        add_custom_target(coverage
            DEPENDS ${ALL_TEST_EXECUTABLES}
            COMMENT "🔍 Running llvm-cov → HTML coverage report (Clang/LLVM)"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            # COMMAND ${CMAKE_COMMAND} -E env LLVM_PROFILE_FILE=coverage_%p.profraw ctest
            COMMAND ${LLVM_PROFDATA_EXECUTABLE} merge -sparse test/coverage_*.profraw -o coverage.profdata
            COMMAND ${LLVM_COV_EXECUTABLE} show
                    ${ALL_TEST_EXECUTABLES}
                    -instr-profile=coverage.profdata
                    -format=html
                    -output-dir=coverage_html
                    ${CMAKE_SOURCE_DIR}/src/derive-c
            COMMENT "Running tests and generating coverage report (clang)"
            VERBATIM
        )
    endif()
endif()
