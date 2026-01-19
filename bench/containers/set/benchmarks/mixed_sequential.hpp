/// @file mixed_sequential.hpp
/// @brief Interleaved set operations
///
/// Checking Regressions For:
/// - Mixed insert/remove/contains workloads
/// - Tombstone handling in open addressing
/// - Size tracking during insert/remove cycles
/// - Rehashing correctness with concurrent queries
/// - Correctness of contains for removed elements
///
/// Representative:
/// Not production representative. Deterministic odd/even pattern exercises
/// tombstone management edge cases not typically seen in profiling.

#pragma once

#include <benchmark/benchmark.h>
#include <cstddef>
#include <stdexcept>

#include "../instances.hpp"
#include "../../../utils/seed.hpp"
#include "../../../utils/label.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#include <derive-cpp/meta/labels.hpp>
#include <derive-cpp/meta/unreachable.hpp>

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
        if (i % 2 == 1) {
            s.insert(i);
        }
        
        if (i > 0) {
            benchmark::DoNotOptimize(s.find(i - 1) != s.end());
        }
        
        if (i % 6 == 3) {
            s.erase(i);
        }
        
        if (i % 10 == 0 && i > 10) {
            for (size_t j = i - 10; j < i; j++) {
                benchmark::DoNotOptimize(s.find(j) != s.end());
            }
        }
    }
}

template <typename Ext>
void mixed_sequential_case_boost_flat(benchmark::State& /* state */, size_t max_n) {
    typename Ext::Self s;

    for (size_t i = 0; i < max_n; i++) {
        if (i % 2 == 1) {
            s.insert(i);
        }
        
        if (i > 0) {
            benchmark::DoNotOptimize(s.find(i - 1) != s.end());
        }
        
        if (i % 6 == 3) {
            s.erase(i);
        }
        
        if (i % 10 == 0 && i > 10) {
            for (size_t j = i - 10; j < i; j++) {
                benchmark::DoNotOptimize(s.find(j) != s.end());
            }
        }
    }
}

template <typename Impl> void mixed_sequential(benchmark::State& state) {
    const std::size_t max_n = static_cast<std::size_t>(state.range(0));

    set_impl_label_with_item<Impl>(state);

    for (auto _ : state) {
        if constexpr (LABEL_CHECK(Impl, derive_c_swiss)) {
            mixed_sequential_case_derive_c_swiss<Impl>(state, max_n);
        } else if constexpr (LABEL_CHECK(Impl, stl_unordered_set)) {
            mixed_sequential_case_stl_unordered_set<Impl>(state, max_n);
        } else if constexpr (LABEL_CHECK(Impl, stl_set)) {
            mixed_sequential_case_stl_set<Impl>(state, max_n);
        } else if constexpr (LABEL_CHECK(Impl, boost_flat)) {
            mixed_sequential_case_boost_flat<Impl>(state, max_n);
        } else {
            static_assert_unreachable<Impl>();
        }
    }
    
    int64_t ops_per_iter = static_cast<int64_t>(max_n) * 2;
    state.SetItemsProcessed(state.iterations() * ops_per_iter);
}

#define BENCH(...)                                                                                \
    BENCHMARK_TEMPLATE(mixed_sequential, __VA_ARGS__)                                         \
        ->RangeMultiplier(2)                                                                  \
        ->Range(1, 1 << 16)                                                                   \
        ->RangeMultiplier(2)                                                                  \
        ->Range(3, 1 << 16)                                                                   \
        ->RangeMultiplier(2)                                                                  \
        ->Range(5, 1 << 16)                                                                   \
        ->RangeMultiplier(2)                                                                  \
        ->Range(7, 1 << 16)

BENCH(Swiss<size_t, size_t_hash>);
BENCH(StdUnorderedSet<size_t, size_t_hash>);
BENCH(StdSet<size_t>);
BENCH(BoostFlat<size_t, size_t_hash>);

#undef BENCH
