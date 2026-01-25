/// @file pathological_rebalance.hpp
/// @brief Pathological rebalancing behavior (worst-case for lazy rebalancing)
///
/// Checking Regressions For:
/// - Repeated rebalancing costs in lazy policies
/// - Worst-case latency spikes from deferred rebalancing
/// - Performance degradation from unbalanced internal structure
/// - Comparison with eager rebalancing strategies
///
/// Representative:
/// NOT production representative. This is a synthetic worst-case designed to
/// expose the downsides of lazy rebalancing by forcing repeated rebalancing.
/// Pattern: fill one side, drain it completely, repeat.

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
void pathological_rebalance_case_derive_c_circular(benchmark::State& /* state */, size_t batch_size,
                                                    size_t num_cycles) {
    typename NS::Self q = NS::Self_new(stdalloc_get_ref());

    for (size_t cycle = 0; cycle < num_cycles; cycle++) {
        // Fill from back
        for (size_t i = 0; i < batch_size; i++) {
            typename NS::Self_item_t item{};
            NS::Self_push_back(&q, item);
        }

        // Drain from front (forces rebalancing if back is empty)
        for (size_t i = 0; i < batch_size; i++) {
            typename NS::Self_item_t item = NS::Self_pop_front(&q);
            benchmark::DoNotOptimize(item);
        }
    }

    NS::Self_delete(&q);
}

template <QueueCase NS>
void pathological_rebalance_case_derive_c_deque(benchmark::State& /* state */, size_t batch_size,
                                                 size_t num_cycles) {
    typename NS::Self q = NS::Self_new(stdalloc_get_ref());

    for (size_t cycle = 0; cycle < num_cycles; cycle++) {
        // Fill from back
        for (size_t i = 0; i < batch_size; i++) {
            typename NS::Self_item_t item{};
            NS::Self_push_back(&q, item);
        }

        // Drain from front (forces rebalancing when front empties)
        for (size_t i = 0; i < batch_size; i++) {
            typename NS::Self_item_t item = NS::Self_pop_front(&q);
            benchmark::DoNotOptimize(item);
        }
    }

    NS::Self_delete(&q);
}

template <QueueCase Impl>
void pathological_rebalance_case_stl_deque(benchmark::State& /* state */, size_t batch_size,
                                            size_t num_cycles) {
    typename Impl::Self q;

    for (size_t cycle = 0; cycle < num_cycles; cycle++) {
        // Fill from back
        for (size_t i = 0; i < batch_size; i++) {
            typename Impl::Self_item_t item{};
            q.push_back(item);
        }

        // Drain from front
        for (size_t i = 0; i < batch_size; i++) {
            typename Impl::Self_item_t item = q.front();
            q.pop_front();
            benchmark::DoNotOptimize(item);
        }
    }
}

template <QueueCase Impl>
void pathological_rebalance_case_stl_queue(benchmark::State& /* state */, size_t batch_size,
                                            size_t num_cycles) {
    typename Impl::Self q;

    for (size_t cycle = 0; cycle < num_cycles; cycle++) {
        // Fill from back
        for (size_t i = 0; i < batch_size; i++) {
            typename Impl::Self_item_t item{};
            q.push(item);
        }

        // Drain from front
        for (size_t i = 0; i < batch_size; i++) {
            typename Impl::Self_item_t item = q.front();
            q.pop();
            benchmark::DoNotOptimize(item);
        }
    }
}

template <QueueCase Impl> void pathological_rebalance(benchmark::State& state) {
    const std::size_t batch_size = static_cast<std::size_t>(state.range(0));
    const std::size_t num_cycles = 100; // Fixed number of fill/drain cycles

    for (auto _ : state) {
        if constexpr (LABEL_CHECK(Impl, derive_c_circular)) {
            pathological_rebalance_case_derive_c_circular<Impl>(state, batch_size, num_cycles);
        } else if constexpr (LABEL_CHECK(Impl, derive_c_deque)) {
            pathological_rebalance_case_derive_c_deque<Impl>(state, batch_size, num_cycles);
        } else if constexpr (LABEL_CHECK(Impl, stl_deque)) {
            pathological_rebalance_case_stl_deque<Impl>(state, batch_size, num_cycles);
        } else if constexpr (LABEL_CHECK(Impl, stl_queue)) {
            pathological_rebalance_case_stl_queue<Impl>(state, batch_size, num_cycles);
        } else {
            static_assert_unreachable<Impl>();
        }
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batch_size) *
                            static_cast<int64_t>(num_cycles) * 2);
    state.SetLabel(Impl::impl_name);
}

#define BENCH(...)                                                                                \
    BENCHMARK_TEMPLATE(pathological_rebalance, __VA_ARGS__)->Apply(range::exponential<4096>)

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
