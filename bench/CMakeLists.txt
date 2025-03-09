# NOTE: Currently both gcc14 & clang19 have issues with google benchmark
#        - gcc11 is fine? Have not worked out issues (new warnings + regex missing for clang19)
# include(FetchContent)

# FetchContent_Declare(
#     googlebenchmark
#     GIT_REPOSITORY https://github.com/google/benchmark.git
#     GIT_TAG v1.9.1
# )
# FetchContent_MakeAvailable(googlebenchmark)

# file(GLOB BENCH_SOURCES ${CMAKE_SOURCE_DIR}/bench/*.cpp)

# foreach(BENCH_FILE ${BENCH_SOURCES})
#     get_filename_component(BENCH_NAME ${BENCH_FILE} NAME_WE)
#     add_executable(${BENCH_NAME} ${BENCH_FILE})
#     target_link_libraries(${BENCH_NAME} PRIVATE benchmark::benchmark derivec)
# endforeach()
