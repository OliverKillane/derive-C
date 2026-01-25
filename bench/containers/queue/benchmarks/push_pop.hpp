/// @file push_pop.hpp
/// @brief Queue push/pop throughput
///
/// Checking Regressions For:
/// - FIFO push/pop performance
/// - Circular buffer wrap-around overhead
/// - Deque segment allocation and deallocation
/// - Modulo arithmetic in circular queues
/// - Scaling from small to large queue sizes
///
/// Representative:
/// Not production representative. Fill-then-drain pattern tests theoretical
/// throughput, while real queues maintain steady-state with partial drainage.

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
void push_pop_case_derive_c_circular(benchmark::State& /* state */, size_t max_n) {
    typename NS::Self q = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < max_n; i++) {
        typename NS::Self_item_t item{};
        NS::Self_push_back(&q, item);
    }

    while (NS::Self_size(&q) > 0) {
        typename NS::Self_item_t item = NS::Self_pop_front(&q);
        benchmark::DoNotOptimize(&item);
    }

    NS::Self_delete(&q);
}

template <QueueCase NS>
void push_pop_case_derive_c_deque(benchmark::State& /* state */, size_t max_n) {
    typename NS::Self q = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < max_n; i++) {
        typename NS::Self_item_t item{};
        NS::Self_push_back(&q, item);
    }

    while (NS::Self_size(&q) > 0) {
        typename NS::Self_item_t item = NS::Self_pop_front(&q);
        benchmark::DoNotOptimize(&item);
    }

    NS::Self_delete(&q);
}

template <QueueCase Impl>
void push_pop_case_stl_deque(benchmark::State& /* state */, size_t max_n) {
    typename Impl::Self q;

    for (size_t i = 0; i < max_n; i++) {
        typename Impl::Self_item_t item{};
        q.push_back(item);
    }

    while (!q.empty()) {
        typename Impl::Self_item_t item = q.front();
        q.pop_front();
        benchmark::DoNotOptimize(&item);
    }
}

template <QueueCase Impl>
void push_pop_case_stl_queue(benchmark::State& /* state */, size_t max_n) {
    typename Impl::Self q;

    for (size_t i = 0; i < max_n; i++) {
        typename Impl::Self_item_t item{};
        q.push(item);
    }

    while (!q.empty()) {
        typename Impl::Self_item_t item = q.front();
        q.pop();
        benchmark::DoNotOptimize(&item);
    }
}

template <QueueCase Impl> void push_pop(benchmark::State& state) {
    const std::size_t max_n = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        if constexpr (LABEL_CHECK(Impl, derive_c_circular)) {
            push_pop_case_derive_c_circular<Impl>(state, max_n);
        } else if constexpr (LABEL_CHECK(Impl, derive_c_deque)) {
            push_pop_case_derive_c_deque<Impl>(state, max_n);
        } else if constexpr (LABEL_CHECK(Impl, stl_deque)) {
            push_pop_case_stl_deque<Impl>(state, max_n);
        } else if constexpr (LABEL_CHECK(Impl, stl_queue)) {
            push_pop_case_stl_queue<Impl>(state, max_n);
        } else {
            static_assert_unreachable<Impl>();
        }
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(max_n) * 2);
    state.SetLabel(Impl::impl_name);
}

#define BENCH(...)                                                                                \
    BENCHMARK_TEMPLATE(push_pop, __VA_ARGS__)->Apply(range::exponential<65536>)

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
