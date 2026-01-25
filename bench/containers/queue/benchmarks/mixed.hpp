/// @file mixed_ops.hpp
/// @brief Interleaved queue operations
///
/// Checking Regressions For:
/// - Mixed push/pop steady-state behavior
/// - Queue size tracking accuracy during interleaved operations
/// - Circular buffer index management under varying load
/// - Deque chunk recycling with non-monotonic size changes
/// - Asymmetric operation ratios causing queue growth
///
/// Representative:
/// Not production representative. Deterministic modulo pattern creates
/// irregular sequences that exercise edge cases not seen in typical profiling.

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
void mixed_ops_case_derive_c_circular(benchmark::State& /* state */, size_t iterations) {
    typename NS::Self q = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < iterations; i++) {
        if (i % 3 != 2) {
            typename NS::Self_item_t item{};
            NS::Self_push_back(&q, item);
        }
        if (i % 5 == 0 && NS::Self_size(&q) > 0) {
            typename NS::Self_item_t item = NS::Self_pop_front(&q);
            benchmark::DoNotOptimize(item);
        }
    }

    NS::Self_delete(&q);
}

template <QueueCase NS>
void mixed_ops_case_derive_c_deque(benchmark::State& /* state */, size_t iterations) {
    typename NS::Self q = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < iterations; i++) {
        if (i % 3 != 2) {
            typename NS::Self_item_t item{};
            NS::Self_push_back(&q, item);
        }
        if (i % 5 == 0 && NS::Self_size(&q) > 0) {
            typename NS::Self_item_t item = NS::Self_pop_front(&q);
            benchmark::DoNotOptimize(&item);
        }
    }

    NS::Self_delete(&q);
}

template <QueueCase Impl>
void mixed_ops_case_stl_deque(benchmark::State& /* state */, size_t iterations) {
    typename Impl::Self q;

    for (size_t i = 0; i < iterations; i++) {
        if (i % 3 != 2) {
            typename Impl::Self_item_t item{};
            q.push_back(item);
        }
        if (i % 5 == 0 && !q.empty()) {
            typename Impl::Self_item_t item = q.front();
            q.pop_front();
            benchmark::DoNotOptimize(&item);
        }
    }
}

template <QueueCase Impl>
void mixed_ops_case_stl_queue(benchmark::State& /* state */, size_t iterations) {
    typename Impl::Self q;

    for (size_t i = 0; i < iterations; i++) {
        if (i % 3 != 2) {
            typename Impl::Self_item_t item{};
            q.push(item);
        }
        if (i % 5 == 0 && !q.empty()) {
            typename Impl::Self_item_t item = q.front();
            q.pop();
            benchmark::DoNotOptimize(&item);
        }
    }
}

template <QueueCase Impl> void mixed_ops(benchmark::State& state) {
    const std::size_t iterations = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        if constexpr (LABEL_CHECK(Impl, derive_c_circular)) {
            mixed_ops_case_derive_c_circular<Impl>(state, iterations);
        } else if constexpr (LABEL_CHECK(Impl, derive_c_deque)) {
            mixed_ops_case_derive_c_deque<Impl>(state, iterations);
        } else if constexpr (LABEL_CHECK(Impl, stl_deque)) {
            mixed_ops_case_stl_deque<Impl>(state, iterations);
        } else if constexpr (LABEL_CHECK(Impl, stl_queue)) {
            mixed_ops_case_stl_queue<Impl>(state, iterations);
        } else {
            static_assert_unreachable<Impl>();
        }
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(iterations));
    state.SetLabel(Impl::impl_name);
}

#define BENCH(...)                                                                                \
    BENCHMARK_TEMPLATE(mixed_ops, __VA_ARGS__)->Apply(range::exponential<65536>)

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
