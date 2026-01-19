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

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#include <derive-cpp/meta/labels.hpp>
#include <derive-cpp/meta/unreachable.hpp>

static size_t uint32_t_hash(uint32_t const* item) {
    return *item;
}

static size_t uint8_t_hash(uint8_t const* item) {
    return *item;
}

template <typename NS, typename Gen>
void iterate_case_derive_c_swiss(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename NS::Self m = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename NS::Self_key_t, typename NS::Self_value_t>) {
            NS::Self_insert(&m, key, key);
        } else {
            typename NS::Self_value_t value{};
            NS::Self_insert(&m, key, value);
        }
    }

    typename NS::Self_iter_const iter = NS::Self_get_iter_const(&m);
    while (!NS::Self_iter_const_empty(&iter)) {
        typename NS::Self_iter_const_item entry = NS::Self_iter_const_next(&iter);
        benchmark::DoNotOptimize(entry);
    }

    NS::Self_delete(&m);
}

template <typename NS, typename Gen>
void iterate_case_derive_c_ankerl(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename NS::Self m = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename NS::Self_key_t, typename NS::Self_value_t>) {
            NS::Self_insert(&m, key, key);
        } else {
            typename NS::Self_value_t value{};
            NS::Self_insert(&m, key, value);
        }
    }

    typename NS::Self_iter_const iter = NS::Self_get_iter_const(&m);
    while (!NS::Self_iter_const_empty(&iter)) {
        typename NS::Self_iter_const_item entry = NS::Self_iter_const_next(&iter);
        benchmark::DoNotOptimize(entry);
    }

    NS::Self_delete(&m);
}

template <typename NS, typename Gen>
void iterate_case_derive_c_decomposed(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename NS::Self m = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename NS::Self_key_t, typename NS::Self_value_t>) {
            NS::Self_insert(&m, key, key);
        } else {
            typename NS::Self_value_t value{};
            NS::Self_insert(&m, key, value);
        }
    }

    typename NS::Self_iter_const iter = NS::Self_get_iter_const(&m);
    while (!NS::Self_iter_const_empty(&iter)) {
        typename NS::Self_iter_const_item entry = NS::Self_iter_const_next(&iter);
        benchmark::DoNotOptimize(entry);
    }

    NS::Self_delete(&m);
}

template <typename NS, typename Gen>
void iterate_case_derive_c_staticlinear(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename NS::Self m = NS::Self_new();

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename NS::Self_key_t, typename NS::Self_value_t>) {
            NS::Self_insert(&m, key, key);
        } else {
            typename NS::Self_value_t value{};
            NS::Self_insert(&m, key, value);
        }
    }

    typename NS::Self_iter_const iter = NS::Self_get_iter_const(&m);
    while (!NS::Self_iter_const_empty(&iter)) {
        typename NS::Self_iter_const_item entry = NS::Self_iter_const_next(&iter);
        benchmark::DoNotOptimize(entry);
    }

    NS::Self_delete(&m);
}

template <typename Std, typename Gen>
void iterate_case_stl_unordered_map(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Std::Self m;

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename Std::Self_key_t, typename Std::Self_value_t>) {
            m.insert({key, key});
        } else {
            typename Std::Self_value_t value{};
            m.insert({key, value});
        }
    }

    for (const auto& entry : m) {
        benchmark::DoNotOptimize(&entry);
    }
}

template <typename Std, typename Gen>
void iterate_case_stl_map(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Std::Self m;

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename Std::Self_key_t, typename Std::Self_value_t>) {
            m.insert({key, key});
        } else {
            typename Std::Self_value_t value{};
            m.insert({key, value});
        }
    }

    for (const auto& entry : m) {
        benchmark::DoNotOptimize(&entry);
    }
}

template <typename Ext, typename Gen>
void iterate_case_ankerl_unordered_dense(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Ext::Self m;

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename Ext::Self_key_t, typename Ext::Self_value_t>) {
            m.insert({key, key});
        } else {
            typename Ext::Self_value_t value{};
            m.insert({key, value});
        }
    }

    for (const auto& entry : m) {
        benchmark::DoNotOptimize(&entry);
    }
}

template <typename Ext, typename Gen>
void iterate_case_abseil_swiss(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Ext::Self m;

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename Ext::Self_key_t, typename Ext::Self_value_t>) {
            m.insert({key, key});
        } else {
            typename Ext::Self_value_t value{};
            m.insert({key, value});
        }
    }

    for (const auto& entry : m) {
        benchmark::DoNotOptimize(&entry);
    }
}

template <typename Ext, typename Gen>
void iterate_case_boost_flat(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Ext::Self m;

    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename Ext::Self_key_t, typename Ext::Self_value_t>) {
            m.insert({key, key});
        } else {
            typename Ext::Self_value_t value{};
            m.insert({key, value});
        }
    }

    for (const auto& entry : m) {
        benchmark::DoNotOptimize(&entry);
    }
}

template <typename Impl> void iterate(benchmark::State& state) {
    const std::size_t max_n = static_cast<std::size_t>(state.range(0));

    set_impl_label_with_key_value<Impl>(state);

    // Select the appropriate generator based on the key type
    if constexpr (std::is_same_v<typename Impl::Self_key_t, std::uint32_t>) {
        for (auto _ : state) {
            U32SeqGen gen(SEED);  // Create fresh generator for each iteration
            if constexpr (LABEL_CHECK(Impl, derive_c_swiss)) {
                iterate_case_derive_c_swiss<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, derive_c_ankerl)) {
                iterate_case_derive_c_ankerl<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, derive_c_decomposed)) {
                iterate_case_derive_c_decomposed<Impl>(state, max_n, gen);
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
            U8SeqGen gen(SEED);  // Create fresh generator for each iteration
            if constexpr (LABEL_CHECK(Impl, derive_c_swiss)) {
                iterate_case_derive_c_swiss<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, derive_c_ankerl)) {
                iterate_case_derive_c_ankerl<Impl>(state, max_n, gen);
            } else if constexpr (LABEL_CHECK(Impl, derive_c_decomposed)) {
                iterate_case_derive_c_decomposed<Impl>(state, max_n, gen);
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
    
    // Report items iterated
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(max_n));
}

// Small sizes for all implementations (including StaticLinear with CAPACITY 1024)
#define BENCH_SMALL(...)                                                                          \
    BENCHMARK_TEMPLATE(iterate, __VA_ARGS__)                                                      \
        ->RangeMultiplier(2)                                                                      \
        ->Range(1, 1024)                                                                          \
        ->RangeMultiplier(2)                                                                      \
        ->Range(3, 1024)                                                                          \
        ->RangeMultiplier(2)                                                                      \
        ->Range(5, 1024)                                                                          \
        ->RangeMultiplier(2)                                                                      \
        ->Range(7, 1024)

// Large sizes for dynamic implementations (excluding StaticLinear)
#define BENCH_LARGE(...)                                                                          \
    BENCHMARK_TEMPLATE(iterate, __VA_ARGS__)                                                      \
        ->RangeMultiplier(4)                                                                      \
        ->Range(4096, 65536)                                                                      \
        ->RangeMultiplier(4)                                                                      \
        ->Range(6144, 65536)                                                                      \
        ->RangeMultiplier(4)                                                                      \
        ->Range(5120, 65536)                                                                      \
        ->RangeMultiplier(4)                                                                      \
        ->Range(7168, 65536)

// Small sizes for uint8_t (max 256 unique keys)
#define BENCH_U8(...)                                                                             \
    BENCHMARK_TEMPLATE(iterate, __VA_ARGS__)                                                      \
        ->RangeMultiplier(2)                                                                      \
        ->Range(1, 256)                                                                           \
        ->RangeMultiplier(2)                                                                      \
        ->Range(3, 256)                                                                           \
        ->RangeMultiplier(2)                                                                      \
        ->Range(5, 256)                                                                           \
        ->RangeMultiplier(2)                                                                      \
        ->Range(7, 256)

BENCH_SMALL(Swiss<std::uint32_t, std::uint32_t, uint32_t_hash>);
BENCH_LARGE(Swiss<std::uint32_t, std::uint32_t, uint32_t_hash>);

BENCH_SMALL(Ankerl<std::uint32_t, std::uint32_t, uint32_t_hash>);
BENCH_LARGE(Ankerl<std::uint32_t, std::uint32_t, uint32_t_hash>);

BENCH_SMALL(Decomposed<std::uint32_t, std::uint32_t, uint32_t_hash>);
BENCH_LARGE(Decomposed<std::uint32_t, std::uint32_t, uint32_t_hash>);

BENCH_SMALL(StaticLinear<std::uint32_t, std::uint32_t>);

BENCH_SMALL(StdUnorderedMap<std::uint32_t, std::uint32_t, uint32_t_hash>);
BENCH_LARGE(StdUnorderedMap<std::uint32_t, std::uint32_t, uint32_t_hash>);

BENCH_SMALL(StdMap<std::uint32_t, std::uint32_t>);
BENCH_LARGE(StdMap<std::uint32_t, std::uint32_t>);

BENCH_SMALL(AnkerlUnorderedDense<std::uint32_t, std::uint32_t, uint32_t_hash>);
BENCH_LARGE(AnkerlUnorderedDense<std::uint32_t, std::uint32_t, uint32_t_hash>);

BENCH_SMALL(AbseilSwiss<std::uint32_t, std::uint32_t, uint32_t_hash>);
BENCH_LARGE(AbseilSwiss<std::uint32_t, std::uint32_t, uint32_t_hash>);

BENCH_SMALL(BoostFlat<std::uint32_t, std::uint32_t, uint32_t_hash>);
BENCH_LARGE(BoostFlat<std::uint32_t, std::uint32_t, uint32_t_hash>);

BENCH_U8(Swiss<std::uint8_t, std::uint8_t, uint8_t_hash>);

BENCH_U8(Ankerl<std::uint8_t, std::uint8_t, uint8_t_hash>);

BENCH_U8(Decomposed<std::uint8_t, std::uint8_t, uint8_t_hash>);

BENCH_U8(StaticLinear<std::uint8_t, std::uint8_t>);

BENCH_U8(StdUnorderedMap<std::uint8_t, std::uint8_t, uint8_t_hash>);

BENCH_U8(StdMap<std::uint8_t, std::uint8_t>);

BENCH_U8(AnkerlUnorderedDense<std::uint8_t, std::uint8_t, uint8_t_hash>);

BENCH_U8(AbseilSwiss<std::uint8_t, std::uint8_t, uint8_t_hash>);

BENCH_U8(BoostFlat<std::uint8_t, std::uint8_t, uint8_t_hash>);

BENCH_SMALL(Swiss<std::uint32_t, Bytes<32>, uint32_t_hash>);
BENCH_LARGE(Swiss<std::uint32_t, Bytes<32>, uint32_t_hash>);

BENCH_SMALL(Ankerl<std::uint32_t, Bytes<32>, uint32_t_hash>);
BENCH_LARGE(Ankerl<std::uint32_t, Bytes<32>, uint32_t_hash>);

BENCH_SMALL(Decomposed<std::uint32_t, Bytes<32>, uint32_t_hash>);
BENCH_LARGE(Decomposed<std::uint32_t, Bytes<32>, uint32_t_hash>);

BENCH_SMALL(StaticLinear<std::uint32_t, Bytes<32>>);

BENCH_SMALL(StdUnorderedMap<std::uint32_t, Bytes<32>, uint32_t_hash>);
BENCH_LARGE(StdUnorderedMap<std::uint32_t, Bytes<32>, uint32_t_hash>);

BENCH_SMALL(StdMap<std::uint32_t, Bytes<32>>);
BENCH_LARGE(StdMap<std::uint32_t, Bytes<32>>);

BENCH_SMALL(AnkerlUnorderedDense<std::uint32_t, Bytes<32>, uint32_t_hash>);
BENCH_LARGE(AnkerlUnorderedDense<std::uint32_t, Bytes<32>, uint32_t_hash>);

BENCH_SMALL(AbseilSwiss<std::uint32_t, Bytes<32>, uint32_t_hash>);
BENCH_LARGE(AbseilSwiss<std::uint32_t, Bytes<32>, uint32_t_hash>);

BENCH_SMALL(BoostFlat<std::uint32_t, Bytes<32>, uint32_t_hash>);
BENCH_LARGE(BoostFlat<std::uint32_t, Bytes<32>, uint32_t_hash>);

#undef BENCH_SMALL
#undef BENCH_LARGE
#undef BENCH_U8
