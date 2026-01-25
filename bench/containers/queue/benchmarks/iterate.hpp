/// @file iterate.hpp
/// @brief Queue iteration performance
///
/// Checking Regressions For:
/// - Iterator-based traversal performance
/// - Circular buffer wrap-around correctness
/// - Deque segment traversal across chunk boundaries
/// - Iterator bounds checking (off-by-one errors)
/// - Iterator overhead vs direct indexing
///
/// Representative:
/// Not production representative. Full traversal without modification is
/// uncommon and tests iterator correctness in isolation.

#pragma once

#include <benchmark/benchmark.h>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "../instances.hpp"
#include "../../../utils/seed.hpp"
#include "../../../utils/generator.hpp"
#include "../../../utils/object.hpp"
#include "../../../utils/range.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#include <derive-cpp/meta/labels.hpp>
#include <derive-cpp/meta/unreachable.hpp>

template <QueueCase NS>
void iterate_case_derive_c_circular(benchmark::State& /* state */, size_t max_n) {
    typename NS::Self q = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < max_n; i++) {
        typename NS::Self_item_t item{};
        NS::Self_push_back(&q, item);
    }

    typename NS::Self_iter_const iter = NS::Self_get_iter_const(&q);
    while (!NS::Self_iter_const_empty(&iter)) {
        typename NS::Self_item_t const* item = NS::Self_iter_const_next(&iter);
        benchmark::DoNotOptimize(&item);
    }

    NS::Self_delete(&q);
}

template <typename NS>
void iterate_case_derive_c_deque(benchmark::State& /* state */, size_t max_n) {
    typename NS::Self q = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < max_n; i++) {
        typename NS::Self_item_t item{};
        NS::Self_push_back(&q, item);
    }

    typename NS::Self_iter_const iter = NS::Self_get_iter_const(&q);
    while (!NS::Self_iter_const_empty(&iter)) {
        typename NS::Self_item_t const* item = NS::Self_iter_const_next(&iter);
        benchmark::DoNotOptimize(item);
    }

    NS::Self_delete(&q);
}

template <QueueCase Impl>
void iterate_case_stl_deque(benchmark::State& /* state */, size_t max_n) {
    typename Impl::Self q;

    for (size_t i = 0; i < max_n; i++) {
        typename Impl::Self_item_t item{};
        q.push_back(item);
    }

    for (auto const& item : q) {
        benchmark::DoNotOptimize(&item);
    }
}

template <QueueCase Impl>
void iterate_case_stl_queue(benchmark::State& /* state */, size_t max_n) {
    typename Impl::Self q;

    for (size_t i = 0; i < max_n; i++) {
        typename Impl::Self_item_t item{};
        q.push(item);
    }

    // std::queue doesn't support iteration, so we pop all
    while (!q.empty()) {
        auto const& item = q.front();
        benchmark::DoNotOptimize(&item);
        q.pop();
    }
}

template <QueueCase Impl> void iterate(benchmark::State& state) {
    const std::size_t max_n = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        if constexpr (LABEL_CHECK(Impl, derive_c_circular)) {
            iterate_case_derive_c_circular<Impl>(state, max_n);
        } else if constexpr (LABEL_CHECK(Impl, derive_c_deque)) {
            iterate_case_derive_c_deque<Impl>(state, max_n);
        } else if constexpr (LABEL_CHECK(Impl, stl_deque)) {
            iterate_case_stl_deque<Impl>(state, max_n);
        } else if constexpr (LABEL_CHECK(Impl, stl_queue)) {
            iterate_case_stl_queue<Impl>(state, max_n);
        } else {
            static_assert_unreachable<Impl>();
        }
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(max_n));
    state.SetLabel(Impl::impl_name);
}

#define BENCH(...)                                                                                \
    BENCHMARK_TEMPLATE(iterate, __VA_ARGS__)->Apply(range::exponential<65536>)

// uint8_t benchmarks
BENCH(Circular<std::uint8_t>);
BENCH(Deque<std::uint8_t>);
BENCH(StdDeque<std::uint8_t>);
BENCH(StdQueue<std::uint8_t>);

// 16-byte object benchmarks
BENCH(Circular<Bytes<16>>);
BENCH(Deque<Bytes<16>>);
BENCH(StdDeque<Bytes<16>>);
BENCH(StdQueue<Bytes<16>>);

#undef BENCH
