/// @file lookup.hpp
/// @brief Lookup operations on maps
///
/// Checking Regressions For:
/// - Lookup performance after insertion
/// - Hash table lookup efficiency
/// - Decomposed vs paired storage lookup overhead
/// - StaticLinear lookup performance
/// - Lookup performance with different key/value sizes
///
/// Representative:
/// Not production representative. Insert-all-then-lookup-all pattern tests
/// lookup performance in isolation, while real code interleaves operations.

#pragma once

#include <benchmark/benchmark.h>
#include <cstddef>
#include <cstdint>
#include <type_traits>

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
void lookup_case_derive_c(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename NS::Self m = NS::Self_new(stdalloc_get_ref());

    // Insert phase
    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename NS::Self_key_t, typename NS::Self_value_t>) {
            NS::Self_insert(&m, key, key);
        } else {
            typename NS::Self_value_t value{};
            NS::Self_insert(&m, key, value);
        }
    }

    // Lookup phase - re-seed generator to get same keys
    Gen lookup_gen(SEED);
    for (size_t i = 0; i < max_n; i++) {
        auto key = lookup_gen.next();
        auto* value = NS::Self_read(&m, key);
        benchmark::DoNotOptimize(value);
    }

    NS::Self_delete(&m);
}

template <MapCase NS, typename Gen>
void lookup_case_derive_c_staticlinear(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename NS::Self m = NS::Self_new();

    // Insert phase
    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename NS::Self_key_t, typename NS::Self_value_t>) {
            NS::Self_insert(&m, key, key);
        } else {
            typename NS::Self_value_t value{};
            NS::Self_insert(&m, key, value);
        }
    }

    // Lookup phase - re-seed generator to get same keys
    Gen lookup_gen(SEED);
    for (size_t i = 0; i < max_n; i++) {
        auto key = lookup_gen.next();
        auto* value = NS::Self_read(&m, key);
        benchmark::DoNotOptimize(value);
    }

    NS::Self_delete(&m);
}

template <MapCase Std, typename Gen>
void lookup_case_stl_unordered_map(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Std::Self m;

    // Insert phase
    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename Std::Self_key_t, typename Std::Self_value_t>) {
            m.insert({key, key});
        } else {
            typename Std::Self_value_t value{};
            m.insert({key, value});
        }
    }

    // Lookup phase - re-seed generator to get same keys
    Gen lookup_gen(SEED);
    for (size_t i = 0; i < max_n; i++) {
        auto key = lookup_gen.next();
        auto it = m.find(key);
        benchmark::DoNotOptimize(&it);
    }
}

template <MapCase Std, typename Gen>
void lookup_case_stl_map(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Std::Self m;

    // Insert phase
    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename Std::Self_key_t, typename Std::Self_value_t>) {
            m.insert({key, key});
        } else {
            typename Std::Self_value_t value{};
            m.insert({key, value});
        }
    }

    // Lookup phase - re-seed generator to get same keys
    Gen lookup_gen(SEED);
    for (size_t i = 0; i < max_n; i++) {
        auto key = lookup_gen.next();
        auto it = m.find(key);
        benchmark::DoNotOptimize(&it);
    }
}

template <MapCase Ext, typename Gen>
void lookup_case_ankerl_unordered_dense(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Ext::Self m;

    // Insert phase
    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename Ext::Self_key_t, typename Ext::Self_value_t>) {
            m.insert({key, key});
        } else {
            typename Ext::Self_value_t value{};
            m.insert({key, value});
        }
    }

    // Lookup phase - re-seed generator to get same keys
    Gen lookup_gen(SEED);
    for (size_t i = 0; i < max_n; i++) {
        auto key = lookup_gen.next();
        auto it = m.find(key);
        benchmark::DoNotOptimize(&it);
    }
}

template <MapCase Ext, typename Gen>
void lookup_case_abseil_swiss(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Ext::Self m;

    // Insert phase
    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename Ext::Self_key_t, typename Ext::Self_value_t>) {
            m.insert({key, key});
        } else {
            typename Ext::Self_value_t value{};
            m.insert({key, value});
        }
    }

    // Lookup phase - re-seed generator to get same keys
    Gen lookup_gen(SEED);
    for (size_t i = 0; i < max_n; i++) {
        auto key = lookup_gen.next();
        auto it = m.find(key);
        benchmark::DoNotOptimize(&it);
    }
}

template <MapCase Ext, typename Gen>
void lookup_case_boost_flat(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Ext::Self m;

    // Insert phase
    for (size_t i = 0; i < max_n; i++) {
        auto key = gen.next();
        if constexpr (std::is_same_v<typename Ext::Self_key_t, typename Ext::Self_value_t>) {
            m.insert({key, key});
        } else {
            typename Ext::Self_value_t value{};
            m.insert({key, value});
        }
    }

    // Lookup phase - re-seed generator to get same keys
    Gen lookup_gen(SEED);
    for (size_t i = 0; i < max_n; i++) {
        auto key = lookup_gen.next();
        auto it = m.find(key);
        benchmark::DoNotOptimize(&it);
    }
}

template <MapCase Impl> void lookup(benchmark::State& state) {
    const std::size_t max_n = static_cast<std::size_t>(state.range(0));

    set_impl_label_with_key_value<Impl>(state);

    // Only support uint32_t keys
    static_assert(std::is_same_v<typename Impl::Self_key_t, std::uint32_t>,
                  "Key type must be std::uint32_t");
    for (auto _ : state) {
        SeqGen<std::uint32_t> gen(SEED);  // Create fresh generator for each iteration
        if constexpr (LABEL_CHECK(Impl, derive_c_swiss) || LABEL_CHECK(Impl, derive_c_ankerl) ||
                      LABEL_CHECK(Impl, derive_c_ankerl_small) ||
                      LABEL_CHECK(Impl, derive_c_decomposed)) {
            lookup_case_derive_c<Impl>(state, max_n, gen);
        } else if constexpr (LABEL_CHECK(Impl, derive_c_staticlinear)) {
            lookup_case_derive_c_staticlinear<Impl>(state, max_n, gen);
        } else if constexpr (LABEL_CHECK(Impl, stl_unordered_map)) {
            lookup_case_stl_unordered_map<Impl>(state, max_n, gen);
        } else if constexpr (LABEL_CHECK(Impl, stl_map)) {
            lookup_case_stl_map<Impl>(state, max_n, gen);
        } else if constexpr (LABEL_CHECK(Impl, ankerl_unordered_dense)) {
            lookup_case_ankerl_unordered_dense<Impl>(state, max_n, gen);
        } else if constexpr (LABEL_CHECK(Impl, abseil_swiss)) {
            lookup_case_abseil_swiss<Impl>(state, max_n, gen);
        } else if constexpr (LABEL_CHECK(Impl, boost_flat)) {
            lookup_case_boost_flat<Impl>(state, max_n, gen);
        } else {
            static_assert_unreachable<Impl>();
        }
    }

    // Report items processed (lookups only, not inserts)
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(max_n));
}

// Small sizes for StaticLinear (CAPACITY 1024)
#define BENCH_SMALL(...)                                                                          \
    BENCHMARK_TEMPLATE(lookup, __VA_ARGS__)->Apply(range::exponential<1024>)

// Large sizes for dynamic implementations
#define BENCH_LARGE(...)                                                                          \
    BENCHMARK_TEMPLATE(lookup, __VA_ARGS__)->Apply(range::exponential<65536>)

BENCH_LARGE(Swiss<std::uint32_t, Bytes<16>, uint32_t_hash_id>);
BENCH_LARGE(Ankerl<std::uint32_t, Bytes<16>, uint32_t_hash_id>);
BENCH_LARGE(AnkerlSmall<std::uint32_t, Bytes<16>, uint32_t_hash_id>);
BENCH_LARGE(Decomposed<std::uint32_t, Bytes<16>, uint32_t_hash_id>);
BENCH_SMALL(StaticLinear<std::uint32_t, Bytes<16>>);
BENCH_LARGE(StdUnorderedMap<std::uint32_t, Bytes<16>, uint32_t_hash_id>);
BENCH_LARGE(StdMap<std::uint32_t, Bytes<16>>);
BENCH_LARGE(AnkerlUnorderedDense<std::uint32_t, Bytes<16>, uint32_t_hash_id>);
BENCH_LARGE(AbseilSwiss<std::uint32_t, Bytes<16>, uint32_t_hash_id>);
BENCH_LARGE(BoostFlat<std::uint32_t, Bytes<16>, uint32_t_hash_id>);

#undef BENCH_SMALL
#undef BENCH_LARGE
