/// @file mixed.hpp
/// @brief Vector push and iteration interleaving
///
/// Checking Regressions For:
/// - Reallocation correctness during growth
/// - Quadratic behavior (push one, iterate all)
/// - Dynamic vs hybrid allocator overhead
/// - Internal state validity after reallocation
/// - Copy overhead for different object sizes
///
/// Representative:
/// Not production representative. Iterating after every push is pathological,
/// while real code batches insertions or uses incremental updates.

#pragma once

#include <benchmark/benchmark.h>
#include <cstddef>

#include "../instances.hpp"
#include "../../../utils/seed.hpp"
#include "../../../utils/generator.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#include <derive-cpp/meta/labels.hpp>
#include <derive-cpp/meta/unreachable.hpp>

template <typename NS, typename Gen>
void mixed_case_derive_c_dynamic(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename NS::Self v = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < max_n; i++) {
        NS::Self_push(&v, gen.next());

        typename NS::Self_iter_const iter = NS::Self_get_iter_const(&v);
        while (!NS::Self_iter_const_empty(&iter)) {
            typename NS::Self_iter_const_item entry = NS::Self_iter_const_next(&iter);
            benchmark::DoNotOptimize(entry);
        }
    }

    NS::Self_delete(&v);
}

template <typename NS, typename Gen>
void mixed_case_derive_c_hybrid(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename NS::HybridAlloc_buffer buffer = {};
    typename NS::HybridAlloc alloc = NS::HybridAlloc_new(&buffer, stdalloc_get_ref());

    typename NS::Self v = NS::Self_new(&alloc);

    for (size_t i = 0; i < max_n; i++) {
        NS::Self_push(&v, gen.next());

        typename NS::Self_iter_const iter = NS::Self_get_iter_const(&v);
        while (!NS::Self_iter_const_empty(&iter)) {
            typename NS::Self_iter_const_item entry = NS::Self_iter_const_next(&iter);
            benchmark::DoNotOptimize(entry);
        }
    }

    NS::Self_delete(&v);
}

template <typename Std, typename Gen>
void mixed_case_stl(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Std::Self x;

    for (size_t i = 0; i < max_n; i++) {
        x.push_back(gen.next());

        for (const auto& entry : x) {
            benchmark::DoNotOptimize(&entry);
        }
    }
}

template <typename Impl> void mixed(benchmark::State& state) {
    const std::size_t max_n = static_cast<std::size_t>(state.range(0));

    BytesConstGen<typename Impl::Self_item_t{}> gen(SEED);

    for (auto _ : state) {
        if constexpr (LABEL_CHECK(Impl, derive_c_dynamic)) {
            mixed_case_derive_c_dynamic<Impl>(state, max_n, gen);
        } else if constexpr (LABEL_CHECK(Impl, derive_c_hybrid)) {
            mixed_case_derive_c_hybrid<Impl>(state, max_n, gen);
        } else if constexpr (LABEL_CHECK(Impl, stl_vector)) {
            mixed_case_stl<Impl>(state, max_n, gen);
        } else {
            static_assert_unreachable<Impl>();
        }
    }
    
    // Report push operations (1 push per iteration of the loop)
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(max_n));
    state.SetLabel("pushes+iters");
}

#define BENCH(...)                                                                                \
    BENCHMARK_TEMPLATE(mixed, __VA_ARGS__)                                                    \
        ->RangeMultiplier(2)                                                                  \
        ->Range(1, 1 << 18)                                                                   \
        ->RangeMultiplier(2)                                                                  \
        ->Range(3, 1 << 18)                                                                   \
        ->RangeMultiplier(2)                                                                  \
        ->Range(5, 1 << 18)                                                                   \
        ->RangeMultiplier(2)                                                                  \
        ->Range(7, 1 << 18)

BENCH(Std<Bytes<1>>);
BENCH(Dynamic<Bytes<1>>);
BENCH(Hybrid<Bytes<1>>);

#undef BENCH