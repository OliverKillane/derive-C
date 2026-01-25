#pragma once

#include <benchmark/benchmark.h>

namespace range {

#ifdef BENCH_FINE_GRAINED
static constexpr bool fine_grained = true;
#else
static constexpr bool fine_grained = false;
#endif

template <int64_t Max>
inline void exponential(benchmark::internal::Benchmark* benchmark) {
    // Benchmark for single item case
    benchmark = benchmark->Arg(1);

    if constexpr (fine_grained) {
        int64_t value = 2;
        while (value <= Max) {
            benchmark = benchmark->Arg(value);
            value *= 8;
        }
    } else {
        // JUSTIFY: Using a small set of primes
        // - We want to see performance over a lare range (so logarithmic x-axis of sale factor)
        // - We want a large range of sizes (e.g. not just powers of 2), to observe performance over a
        // large range of values
        for (const int32_t base : {2, 3, 5}) {
            for (const int32_t multiplier : {2, 3}) {
                int64_t value = base;
                while (value <= Max) {
                    benchmark = benchmark->Arg(value);
                    value *= multiplier;
                }
            }
        }
    }
}

} // namespace range