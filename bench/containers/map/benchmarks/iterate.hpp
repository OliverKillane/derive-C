/// @file iterate.hpp
/// @brief Map iteration after insertion
///
/// Checking Regressions For:
/// - Key-value pair iteration performance
/// - Iterator correctness with sequential keys (worst-case collisions)
/// - Decomposed storage vs paired storage layouts
/// - StaticLinear iteration up to capacity vs size
/// - Limited key space behavior (uint8_t wraparound)
///
/// Representative:
/// Not production representative. Full iteration without modification and
/// sequential keys create pathological collision patterns.

#pragma once

#include <benchmark/benchmark.h>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "../instances.hpp"
#include "../../../utils/seed.hpp"
#include "../../../utils/generator.hpp"
#include "../../../utils/object.hpp"
#include "../../../utils/label.hpp"
#include "../../../utils/range.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>
#include <derive-c/algorithm/hash/id.h>

#include <derive-cpp/meta/labels.hpp>
#include <derive-cpp/meta/unreachable.hpp>

template <MapCase NS, typename Gen>
void iterate_case_derive_c(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename NS::Self m = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        typename NS::Self_value_t value{};
        NS::Self_try_insert(&m, key, value);
    }

    typename NS::Self_iter_const iter = NS::Self_get_iter_const(&m);
    while (!NS::Self_iter_const_empty(&iter)) {
        typename NS::Self_iter_const_item entry = NS::Self_iter_const_next(&iter);
        benchmark::DoNotOptimize(entry);
    }

    NS::Self_delete(&m);
}

template <MapCase NS, typename Gen>
void iterate_case_derive_c_staticlinear(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename NS::Self m = NS::Self_new();

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        typename NS::Self_value_t value{};
        NS::Self_try_insert(&m, key, value);
    }

    typename NS::Self_iter_const iter = NS::Self_get_iter_const(&m);
    while (!NS::Self_iter_const_empty(&iter)) {
        typename NS::Self_iter_const_item entry = NS::Self_iter_const_next(&iter);
        benchmark::DoNotOptimize(entry);
    }

    NS::Self_delete(&m);
}

template <MapCase Std, typename Gen>
void iterate_case_stl_unordered_map(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Std::Self m;

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        typename Std::Self_value_t value{};
        m.insert({key, value});
    }

    for (const auto& entry : m) {
        benchmark::DoNotOptimize(&entry);
    }
}

template <MapCase Std, typename Gen>
void iterate_case_stl_map(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Std::Self m;

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        typename Std::Self_value_t value{};
        m.insert({key, value});
    }

    for (const auto& entry : m) {
        benchmark::DoNotOptimize(&entry);
    }
}

template <MapCase Ext, typename Gen>
void iterate_case_ankerl_unordered_dense(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Ext::Self m;

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        typename Ext::Self_value_t value{};
        m.insert({key, value});
    }

    for (const auto& entry : m) {
        benchmark::DoNotOptimize(&entry);
    }
}

template <MapCase Ext, typename Gen>
void iterate_case_abseil_swiss(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Ext::Self m;

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        typename Ext::Self_value_t value{};
        m.insert({key, value});
    }

    for (const auto& entry : m) {
        benchmark::DoNotOptimize(&entry);
    }
}

template <MapCase Ext, typename Gen>
void iterate_case_boost_flat(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Ext::Self m;

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        typename Ext::Self_value_t value{};
        m.insert({key, value});
    }

    for (const auto& entry : m) {
        benchmark::DoNotOptimize(&entry);
    }
}

template <MapCase Impl> void iterate(benchmark::State& state) {
    const std::size_t max_n = static_cast<std::size_t>(state.range(0));

    set_impl_label_with_key_value<Impl>(state);

    // Select the appropriate generator based on the key type
    if constexpr (std::is_same_v<typename Impl::Self_key_t, std::uint32_t>) {
        for (auto _ : state) {
            SeqGen<std::uint32_t> gen(SEED); // Create fresh generator for each iteration
            if constexpr (LABEL_CHECK(Impl, derive_c_swiss) || LABEL_CHECK(Impl, derive_c_ankerl) ||
                          LABEL_CHECK(Impl, derive_c_ankerl_small) ||
                          LABEL_CHECK(Impl, derive_c_decomposed)) {
                iterate_case_derive_c<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, derive_c_staticlinear)) {
                iterate_case_derive_c_staticlinear<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, stl_unordered_map)) {
                iterate_case_stl_unordered_map<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, stl_map)) {
                iterate_case_stl_map<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, ankerl_unordered_dense)) {
                iterate_case_ankerl_unordered_dense<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, abseil_swiss)) {
                iterate_case_abseil_swiss<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, boost_flat)) {
                iterate_case_boost_flat<Impl>(state, max_n, gen);
            } else {
                static_assert_unreachable<Impl>();
            }
        }
    } else if constexpr (std::is_same_v<typename Impl::Self_key_t, std::uint8_t>) {
        for (auto _ : state) {
            SeqGen<std::uint8_t> gen(SEED); // Create fresh generator for each iteration
            if constexpr (LABEL_CHECK(Impl, derive_c_swiss) || LABEL_CHECK(Impl, derive_c_ankerl) ||
                          LABEL_CHECK(Impl, derive_c_ankerl_small) ||
                          LABEL_CHECK(Impl, derive_c_decomposed)) {
                iterate_case_derive_c<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, derive_c_staticlinear)) {
                iterate_case_derive_c_staticlinear<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, stl_unordered_map)) {
                iterate_case_stl_unordered_map<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, stl_map)) {
                iterate_case_stl_map<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, ankerl_unordered_dense)) {
                iterate_case_ankerl_unordered_dense<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, abseil_swiss)) {
                iterate_case_abseil_swiss<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, boost_flat)) {
                iterate_case_boost_flat<Impl>(state, max_n, gen);
            } else {
                static_assert_unreachable<Impl>();
            }
        }
    } else {
        static_assert_unreachable<typename Impl::Self_key_t>();
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(max_n));
}

#define BENCH_CASE(NAME)                                                                           \
    BENCHMARK_TEMPLATE(iterate, NAME<std::uint32_t, std::uint32_t, uint32_t_hash_id>)              \
        ->Apply(range::exponential<                                                                \
                NAME<std::uint32_t, std::uint32_t, uint32_t_hash_id>::Self_max_capacity>);         \
    BENCHMARK_TEMPLATE(iterate, NAME<std::uint8_t, std::uint8_t, uint8_t_hash_id>)                 \
        ->Apply(range::exponential<                                                                \
                NAME<std::uint8_t, std::uint8_t, uint8_t_hash_id>::Self_max_capacity>);            \
    BENCHMARK_TEMPLATE(iterate, NAME<std::uint32_t, Bytes<32>, uint32_t_hash_id>)                  \
        ->Apply(range::exponential<                                                                \
                NAME<std::uint32_t, Bytes<32>, uint32_t_hash_id>::Self_max_capacity>)

APPLY_BENCH(BENCH_CASE);

#undef BENCH_CASE
