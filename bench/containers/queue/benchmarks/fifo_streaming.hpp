/// @file fifo_streaming.hpp
/// @brief Streaming FIFO queue operations (worst-case for lazy rebalancing)
///
/// Checking Regressions For:
/// - Steady-state FIFO performance with maintained queue size
/// - Rebalancing behavior during continuous push_back/pop_front
/// - Latency spikes from deferred rebalancing
/// - Cache effects from unbalanced internal structure
/// - Allocation/deallocation patterns during streaming
///
/// Representative:
/// Highly production representative. Models real queue workloads where
/// elements are continuously added and removed, maintaining a steady working set.
/// This pattern exposes worst-case behavior for lazy rebalancing policies.

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
#include <derive-cpp/meta/unreachable.hpp>

template <typename NS>
void fifo_streaming_case_derive_c_circular(benchmark::State& /* state */, size_t steady_state_size,
                                            size_t total_ops) {
    typename NS::Self q = NS::Self_new(stdalloc_get_ref());

    // Fill to steady state
    for (size_t i = 0; i < steady_state_size; i++) {
        typename NS::Self_item_t item{};
        NS::Self_push_back(&q, item);
    }

    // Streaming: pop one, push one, maintaining steady state
    for (size_t i = 0; i < total_ops; i++) {
        typename NS::Self_item_t item = NS::Self_pop_front(&q);
        benchmark::DoNotOptimize(item);

        typename NS::Self_item_t new_item{};
        NS::Self_push_back(&q, new_item);
    }

    NS::Self_delete(&q);
}

template <typename NS>
void fifo_streaming_case_derive_c_deque(benchmark::State& /* state */, size_t steady_state_size,
                                         size_t total_ops) {
    typename NS::Self q = NS::Self_new(stdalloc_get_ref());

    // Fill to steady state
    for (size_t i = 0; i < steady_state_size; i++) {
        typename NS::Self_item_t item{};
        NS::Self_push_back(&q, item);
    }

    // Streaming: pop one, push one, maintaining steady state
    for (size_t i = 0; i < total_ops; i++) {
        typename NS::Self_item_t item = NS::Self_pop_front(&q);
        benchmark::DoNotOptimize(item);

        typename NS::Self_item_t new_item{};
        NS::Self_push_back(&q, new_item);
    }

    NS::Self_delete(&q);
}

template <typename Impl>
void fifo_streaming_case_stl_deque(benchmark::State& /* state */, size_t steady_state_size,
                                    size_t total_ops) {
    typename Impl::Self q;

    // Fill to steady state
    for (size_t i = 0; i < steady_state_size; i++) {
        typename Impl::Self_item_t item{};
        q.push_back(item);
    }

    // Streaming: pop one, push one, maintaining steady state
    for (size_t i = 0; i < total_ops; i++) {
        typename Impl::Self_item_t item = q.front();
        q.pop_front();
        benchmark::DoNotOptimize(item);

        typename Impl::Self_item_t new_item{};
        q.push_back(new_item);
    }
}

template <typename Impl>
void fifo_streaming_case_stl_queue(benchmark::State& /* state */, size_t steady_state_size,
                                    size_t total_ops) {
    typename Impl::Self q;

    // Fill to steady state
    for (size_t i = 0; i < steady_state_size; i++) {
        typename Impl::Self_item_t item{};
        q.push(item);
    }

    // Streaming: pop one, push one, maintaining steady state
    for (size_t i = 0; i < total_ops; i++) {
        typename Impl::Self_item_t item = q.front();
        q.pop();
        benchmark::DoNotOptimize(item);

        typename Impl::Self_item_t new_item{};
        q.push(new_item);
    }
}

template <typename Impl> void fifo_streaming(benchmark::State& state) {
    const std::size_t steady_state_size = static_cast<std::size_t>(state.range(0));
    const std::size_t total_ops = steady_state_size * 10; // 10x the queue size

    for (auto _ : state) {
        if constexpr (LABEL_CHECK(Impl, derive_c_circular)) {
            fifo_streaming_case_derive_c_circular<Impl>(state, steady_state_size, total_ops);
        } else if constexpr (LABEL_CHECK(Impl, derive_c_deque)) {
            fifo_streaming_case_derive_c_deque<Impl>(state, steady_state_size, total_ops);
        } else if constexpr (LABEL_CHECK(Impl, stl_deque)) {
            fifo_streaming_case_stl_deque<Impl>(state, steady_state_size, total_ops);
        } else if constexpr (LABEL_CHECK(Impl, stl_queue)) {
            fifo_streaming_case_stl_queue<Impl>(state, steady_state_size, total_ops);
        } else {
            static_assert_unreachable<Impl>();
        }
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(total_ops) * 2);
    state.SetLabel(Impl::impl_name);
}

#define BENCH(...)                                                                                \
    BENCHMARK_TEMPLATE(fifo_streaming, __VA_ARGS__)                                           \
        ->RangeMultiplier(2)                                                                  \
        ->Range(1, 1 << 16)                                                                   \
        ->RangeMultiplier(2)                                                                  \
        ->Range(3, 1 << 16)                                                                   \
        ->RangeMultiplier(2)                                                                  \
        ->Range(5, 1 << 16)                                                                   \
        ->RangeMultiplier(2)                                                                  \
        ->Range(7, 1 << 16)

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
