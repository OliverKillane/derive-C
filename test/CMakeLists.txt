include(FetchContent)

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

file(GLOB TEST_SOURCES *.cpp)
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
)