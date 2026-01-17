/// @brief Benchmarking iterator performance after insertion
///  - Insert values up to a given size, then iterate over the entire set

#pragma once

#include <benchmark/benchmark.h>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "../instances.hpp"
#include "../../../utils/seed.hpp"
#include "../../../utils/generator.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#include <derive-cpp/meta/labels.hpp>

// Hash function for uint32_t
static size_t uint32_t_hash(uint32_t const* item) {
    return *item;
}

template <typename NS, typename Gen>
void iterator_case_derive_c_swiss(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename NS::Self s = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < max_n; i++) {
        NS::Self_add(&s, gen.next());
    }

    typename NS::Self_iter_const iter = NS::Self_get_iter_const(&s);
    while (!NS::Self_iter_const_empty(&iter)) {
        typename NS::Self_iter_const_item entry = NS::Self_iter_const_next(&iter);
        benchmark::DoNotOptimize(entry);
    }

    NS::Self_delete(&s);
}

template <typename Std, typename Gen>
void iterator_case_stl_unordered_set(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Std::Self s;

    for (size_t i = 0; i < max_n; i++) {
        s.insert(gen.next());
    }

    for (const auto& entry : s) {
        benchmark::DoNotOptimize(&entry);
    }
}

template <typename Std, typename Gen>
void iterator_case_stl_set(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Std::Self s;

    for (size_t i = 0; i < max_n; i++) {
        s.insert(gen.next());
    }

    for (const auto& entry : s) {
        benchmark::DoNotOptimize(&entry);
    }
}

template <typename Ext, typename Gen>
void iterator_case_boost_flat(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Ext::Self s;

    for (size_t i = 0; i < max_n; i++) {
        s.insert(gen.next());
    }

    for (const auto& entry : s) {
        benchmark::DoNotOptimize(&entry);
    }
}

template <typename Impl> void iterator(benchmark::State& state) {
    const std::size_t max_n = static_cast<std::size_t>(state.range(0));

    U32XORShiftGen gen(SEED);

    for (auto _ : state) {
        if constexpr (LABEL_CHECK(Impl, derive_c_swiss)) {
            iterator_case_derive_c_swiss<Impl>(state, max_n, gen);
        } else if constexpr (LABEL_CHECK(Impl, stl_unordered_set)) {
            iterator_case_stl_unordered_set<Impl>(state, max_n, gen);
        } else if constexpr (LABEL_CHECK(Impl, stl_set)) {
            iterator_case_stl_set<Impl>(state, max_n, gen);
        } else if constexpr (LABEL_CHECK(Impl, boost_flat)) {
            iterator_case_boost_flat<Impl>(state, max_n, gen);
        } else {
            throw std::runtime_error("Unknown implementation type");
        }
    }
    
    // Report items iterated
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(max_n));
    state.SetLabel("iterations");
}

using SwissU32 = Swiss<std::uint32_t, uint32_t_hash>;
using StdUnorderedSetU32 = StdUnorderedSet<std::uint32_t, uint32_t_hash>;
using StdSetU32 = StdSet<std::uint32_t>;
using BoostFlatU32 = BoostFlat<std::uint32_t, uint32_t_hash>;

#define BENCH(IMPL)                                                                                \
    BENCHMARK_TEMPLATE(iterator, IMPL)->Range(1 << 8, 1 << 16);                                   \
    BENCHMARK_TEMPLATE(iterator, IMPL)->Range(1 << 8, 1 << 8);                                    \
    BENCHMARK_TEMPLATE(iterator, IMPL)->Range(1 << 8, 1 << 8)

BENCH(SwissU32);
BENCH(StdUnorderedSetU32);
BENCH(StdSetU32);
BENCH(BoostFlatU32);

#undef BENCH
