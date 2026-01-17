/// @brief Benchmarking iterator performance after insertion
///  - Insert key-value pairs up to a given size, then iterate over the entire map

#pragma once

#include <benchmark/benchmark.h>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "../instances.hpp"
#include "../../../utils/seed.hpp"
#include "../../../utils/generator.hpp"
#include "../../../utils/object.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#include <derive-cpp/meta/labels.hpp>

// Hash function for uint32_t
static size_t uint32_t_hash(uint32_t const* item) {
    return *item;
}

// Hash function for uint8_t
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
                throw std::runtime_error("Unknown implementation type");
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
                throw std::runtime_error("Unknown implementation type");
            }
        }
    } else {
        throw std::runtime_error("Unsupported key type");
    }
    
    // Report items iterated
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(max_n));
    state.SetLabel("iterations");
}

using SwissU32 = Swiss<std::uint32_t, std::uint32_t, uint32_t_hash>;
using AnkerlU32 = Ankerl<std::uint32_t, std::uint32_t, uint32_t_hash>;
using DecomposedU32 = Decomposed<std::uint32_t, std::uint32_t, uint32_t_hash>;
using StaticLinearU32 = StaticLinear<std::uint32_t, std::uint32_t>;
using StdUnorderedMapU32 = StdUnorderedMap<std::uint32_t, std::uint32_t, uint32_t_hash>;
using StdMapU32 = StdMap<std::uint32_t, std::uint32_t>;

using AnkerlUnorderedDenseU32 = AnkerlUnorderedDense<std::uint32_t, std::uint32_t, uint32_t_hash>;
using AbseilSwissU32 = AbseilSwiss<std::uint32_t, std::uint32_t, uint32_t_hash>;
using BoostFlatU32 = BoostFlat<std::uint32_t, std::uint32_t, uint32_t_hash>;

using SwissU8 = Swiss<std::uint8_t, std::uint8_t, uint8_t_hash>;
using AnkerlU8 = Ankerl<std::uint8_t, std::uint8_t, uint8_t_hash>;
using DecomposedU8 = Decomposed<std::uint8_t, std::uint8_t, uint8_t_hash>;
using StaticLinearU8 = StaticLinear<std::uint8_t, std::uint8_t>;
using StdUnorderedMapU8 = StdUnorderedMap<std::uint8_t, std::uint8_t, uint8_t_hash>;
using StdMapU8 = StdMap<std::uint8_t, std::uint8_t>;

using AnkerlUnorderedDenseU8 = AnkerlUnorderedDense<std::uint8_t, std::uint8_t, uint8_t_hash>;
using AbseilSwissU8 = AbseilSwiss<std::uint8_t, std::uint8_t, uint8_t_hash>;
using BoostFlatU8 = BoostFlat<std::uint8_t, std::uint8_t, uint8_t_hash>;

using SwissU32Bytes32 = Swiss<std::uint32_t, Bytes<32>, uint32_t_hash>;
using AnkerlU32Bytes32 = Ankerl<std::uint32_t, Bytes<32>, uint32_t_hash>;
using DecomposedU32Bytes32 = Decomposed<std::uint32_t, Bytes<32>, uint32_t_hash>;
using StaticLinearU32Bytes32 = StaticLinear<std::uint32_t, Bytes<32>>;
using StdUnorderedMapU32Bytes32 = StdUnorderedMap<std::uint32_t, Bytes<32>, uint32_t_hash>;
using StdMapU32Bytes32 = StdMap<std::uint32_t, Bytes<32>>;

using AnkerlUnorderedDenseU32Bytes32 = AnkerlUnorderedDense<std::uint32_t, Bytes<32>, uint32_t_hash>;
using AbseilSwissU32Bytes32 = AbseilSwiss<std::uint32_t, Bytes<32>, uint32_t_hash>;
using BoostFlatU32Bytes32 = BoostFlat<std::uint32_t, Bytes<32>, uint32_t_hash>;

// Small sizes for all implementations (including StaticLinear with CAPACITY 1024)
#define BENCH_SMALL(IMPL)                                                                          \
    BENCHMARK_TEMPLATE(iterate, IMPL)->RangeMultiplier(2)->Range(1, 1024)

// Large sizes for dynamic implementations (excluding StaticLinear)
#define BENCH_LARGE(IMPL)                                                                          \
    BENCHMARK_TEMPLATE(iterate, IMPL)->RangeMultiplier(4)->Range(4096, 65536)

// Small sizes for uint8_t (max 256 unique keys)
#define BENCH_U8(IMPL)                                                                             \
    BENCHMARK_TEMPLATE(iterate, IMPL)->RangeMultiplier(2)->Range(1, 256)

BENCH_SMALL(SwissU32);
BENCH_LARGE(SwissU32);

BENCH_SMALL(AnkerlU32);
BENCH_LARGE(AnkerlU32);

BENCH_SMALL(DecomposedU32);
BENCH_LARGE(DecomposedU32);

BENCH_SMALL(StaticLinearU32);

BENCH_SMALL(StdUnorderedMapU32);
BENCH_LARGE(StdUnorderedMapU32);

BENCH_SMALL(StdMapU32);
BENCH_LARGE(StdMapU32);

BENCH_SMALL(AnkerlUnorderedDenseU32);
BENCH_LARGE(AnkerlUnorderedDenseU32);

BENCH_SMALL(AbseilSwissU32);
BENCH_LARGE(AbseilSwissU32);

BENCH_SMALL(BoostFlatU32);
BENCH_LARGE(BoostFlatU32);

BENCH_U8(SwissU8);

BENCH_U8(AnkerlU8);

BENCH_U8(DecomposedU8);

BENCH_U8(StaticLinearU8);

BENCH_U8(StdUnorderedMapU8);

BENCH_U8(StdMapU8);

BENCH_U8(AnkerlUnorderedDenseU8);

BENCH_U8(AbseilSwissU8);

BENCH_U8(BoostFlatU8);

BENCH_SMALL(SwissU32Bytes32);
BENCH_LARGE(SwissU32Bytes32);

BENCH_SMALL(AnkerlU32Bytes32);
BENCH_LARGE(AnkerlU32Bytes32);

BENCH_SMALL(DecomposedU32Bytes32);
BENCH_LARGE(DecomposedU32Bytes32);

BENCH_SMALL(StaticLinearU32Bytes32);

BENCH_SMALL(StdUnorderedMapU32Bytes32);
BENCH_LARGE(StdUnorderedMapU32Bytes32);

BENCH_SMALL(StdMapU32Bytes32);
BENCH_LARGE(StdMapU32Bytes32);

BENCH_SMALL(AnkerlUnorderedDenseU32Bytes32);
BENCH_LARGE(AnkerlUnorderedDenseU32Bytes32);

BENCH_SMALL(AbseilSwissU32Bytes32);
BENCH_LARGE(AbseilSwissU32Bytes32);

BENCH_SMALL(BoostFlatU32Bytes32);
BENCH_LARGE(BoostFlatU32Bytes32);

#undef BENCH_SMALL
#undef BENCH_LARGE
#undef BENCH_U8
