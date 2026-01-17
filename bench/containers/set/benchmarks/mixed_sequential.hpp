/// @brief Benchmarking mixed insert, contains, and remove operations
///  - Interleaves insert, contains, and remove operations for realistic workload

#pragma once

#include <benchmark/benchmark.h>
#include <cstddef>
#include <stdexcept>

#include "../instances.hpp"
#include "../../../utils/seed.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#include <derive-cpp/meta/labels.hpp>

// Hash function for size_t
static size_t size_t_hash(size_t const* item) {
    return *item;
}

template <typename NS>
void mixed_sequential_case_derive_c_swiss(benchmark::State& /* state */, size_t max_n) {
    typename NS::Self s = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < max_n; i++) {
        // Insert odd numbers
        if (i % 2 == 1) {
            NS::Self_add(&s, i);
        }
        
        // Check contains for previous numbers
        if (i > 0) {
            benchmark::DoNotOptimize(NS::Self_contains(&s, i - 1));
        }
        
        // Remove multiples of 3 (that were inserted)
        if (i % 6 == 3) {  // Odd multiples of 3
            NS::Self_remove(&s, i);
        }
        
        // Check contains for a range
        if (i % 10 == 0 && i > 10) {
            for (size_t j = i - 10; j < i; j++) {
                benchmark::DoNotOptimize(NS::Self_contains(&s, j));
            }
        }
    }

    NS::Self_delete(&s);
}

template <typename Std>
void mixed_sequential_case_stl_unordered_set(benchmark::State& /* state */, size_t max_n) {
    typename Std::Self s;

    for (size_t i = 0; i < max_n; i++) {
        // Insert odd numbers
        if (i % 2 == 1) {
            s.insert(i);
        }
        
        // Check contains for previous numbers
        if (i > 0) {
            benchmark::DoNotOptimize(s.find(i - 1) != s.end());
        }
        
        // Remove multiples of 3 (that were inserted)
        if (i % 6 == 3) {  // Odd multiples of 3
            s.erase(i);
        }
        
        // Check contains for a range
        if (i % 10 == 0 && i > 10) {
            for (size_t j = i - 10; j < i; j++) {
                benchmark::DoNotOptimize(s.find(j) != s.end());
            }
        }
    }
}

template <typename Std>
void mixed_sequential_case_stl_set(benchmark::State& /* state */, size_t max_n) {
    typename Std::Self s;

    for (size_t i = 0; i < max_n; i++) {
        // Insert odd numbers
        if (i % 2 == 1) {
            s.insert(i);
        }
        
        // Check contains for previous numbers
        if (i > 0) {
            benchmark::DoNotOptimize(s.find(i - 1) != s.end());
        }
        
        // Remove multiples of 3 (that were inserted)
        if (i % 6 == 3) {  // Odd multiples of 3
            s.erase(i);
        }
        
        // Check contains for a range
        if (i % 10 == 0 && i > 10) {
            for (size_t j = i - 10; j < i; j++) {
                benchmark::DoNotOptimize(s.find(j) != s.end());
            }
        }
    }
}

template <typename Impl> void mixed_sequential(benchmark::State& state) {
    const std::size_t max_n = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        if constexpr (LABEL_CHECK(Impl, derive_c_swiss)) {
            mixed_sequential_case_derive_c_swiss<Impl>(state, max_n);
        } else if constexpr (LABEL_CHECK(Impl, stl_unordered_set)) {
            mixed_sequential_case_stl_unordered_set<Impl>(state, max_n);
        } else if constexpr (LABEL_CHECK(Impl, stl_set)) {
            mixed_sequential_case_stl_set<Impl>(state, max_n);
        } else {
            throw std::runtime_error("Unknown implementation type");
        }
    }
    
    // Report total operations: inserts (~max_n/2) + contains checks + removes
    // Approximate: max_n/2 inserts, max_n contains, max_n/6 removes
    int64_t ops_per_iter = static_cast<int64_t>(max_n) * 2; // Approximate total ops
    state.SetItemsProcessed(state.iterations() * ops_per_iter);
    state.SetLabel("mixed_ops");
}

using SwissSizeT = Swiss<size_t, size_t_hash>;
using StdUnorderedSetSizeT = StdUnorderedSet<size_t, size_t_hash>;
using StdSetSizeT = StdSet<size_t>;

#define BENCH(IMPL)                                                                                \
    BENCHMARK_TEMPLATE(mixed_sequential, IMPL)->Range(1 << 8, 1 << 16);                           \
    BENCHMARK_TEMPLATE(mixed_sequential, IMPL)->Range(1 << 8, 1 << 8);                            \
    BENCHMARK_TEMPLATE(mixed_sequential, IMPL)->Range(1 << 8, 1 << 8)

BENCH(SwissSizeT);
BENCH(StdUnorderedSetSizeT);
BENCH(StdSetSizeT);

#undef BENCH
