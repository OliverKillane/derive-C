#include <benchmark/benchmark.h>

#include "benchmarks/sequential_alloc.hpp"
#include "benchmarks/mixed_sizes.hpp"
#include "benchmarks/fragmentation.hpp"
#include "benchmarks/zeroed_alloc.hpp"

BENCHMARK_MAIN();
