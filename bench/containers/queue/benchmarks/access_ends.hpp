/// @file access_ends.hpp
/// @brief Queue head/tail access
///
/// Checking Regressions For:
/// - Constant-time front/back access
/// - Circular buffer access across wrap-around boundary
/// - Deque access in first vs last chunk
/// - Correctness of returned elements after wrap-around
/// - Accessor caching and lookup overhead
///
/// Representative:
/// Not production representative. Repeated peek without pop is artificial,
/// while real usage interleaves access with modification.

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
void access_ends_case_derive_c_circular(benchmark::State& /* state */, size_t max_n,
                                        size_t accesses) {
    typename NS::Self q = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < max_n; i++) {
        typename NS::Self_item_t item{};
        NS::Self_push_back(&q, item);
    }

    for (size_t i = 0; i < accesses; i++) {
        typename NS::Self_iter_const iter = NS::Self_get_iter_const(&q);
        typename NS::Self_item_t const* front = NS::Self_iter_const_next(&iter);
        benchmark::DoNotOptimize(&front);

        // Get last element by iterating to end
        typename NS::Self_item_t const* last = nullptr;
        while (!NS::Self_iter_const_empty(&iter)) {
            last = NS::Self_iter_const_next(&iter);
        }
        benchmark::DoNotOptimize(&last);
    }

    NS::Self_delete(&q);
}

template <QueueCase NS>
void access_ends_case_derive_c_deque(benchmark::State& /* state */, size_t max_n,
                                     size_t accesses) {
    typename NS::Self q = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < max_n; i++) {
        typename NS::Self_item_t item{};
        NS::Self_push_back(&q, item);
    }

    for (size_t i = 0; i < accesses; i++) {
        typename NS::Self_iter_const front_iter = NS::Self_get_iter_const(&q);
        typename NS::Self_item_t const* front = NS::Self_iter_const_next(&front_iter);
        benchmark::DoNotOptimize(&front);

        // Get last element by iterating to end
        typename NS::Self_iter_const iter = NS::Self_get_iter_const(&q);
        typename NS::Self_item_t const* last = nullptr;
        while (!NS::Self_iter_const_empty(&iter)) {
            last = NS::Self_iter_const_next(&iter);
        }
        benchmark::DoNotOptimize(&last);
    }

    NS::Self_delete(&q);
}

template <QueueCase Impl>
void access_ends_case_stl_deque(benchmark::State& /* state */, size_t max_n, size_t accesses) {
    typename Impl::Self q;

    for (size_t i = 0; i < max_n; i++) {
        typename Impl::Self_item_t item{};
        q.push_back(item);
    }

    for (size_t i = 0; i < accesses; i++) {
        auto const& front = q.front();
        benchmark::DoNotOptimize(&front);

        auto const& back = q.back();
        benchmark::DoNotOptimize(&back);
    }
}

template <QueueCase Impl>
void access_ends_case_stl_queue(benchmark::State& /* state */, size_t max_n, size_t accesses) {
    typename Impl::Self q;

    for (size_t i = 0; i < max_n; i++) {
        typename Impl::Self_item_t item{};
        q.push(item);
    }

    for (size_t i = 0; i < accesses; i++) {
        auto const& front = q.front();
        benchmark::DoNotOptimize(&front);

        auto const& back = q.back();
        benchmark::DoNotOptimize(&back);
    }
}

template <QueueCase Impl> void access_ends(benchmark::State& state) {
    const std::size_t max_n = static_cast<std::size_t>(state.range(0));
    const std::size_t accesses = 1000;

    for (auto _ : state) {
        if constexpr (LABEL_CHECK(Impl, derive_c_circular)) {
            access_ends_case_derive_c_circular<Impl>(state, max_n, accesses);
        } else if constexpr (LABEL_CHECK(Impl, derive_c_deque)) {
            access_ends_case_derive_c_deque<Impl>(state, max_n, accesses);
        } else if constexpr (LABEL_CHECK(Impl, stl_deque)) {
            access_ends_case_stl_deque<Impl>(state, max_n, accesses);
        } else if constexpr (LABEL_CHECK(Impl, stl_queue)) {
            access_ends_case_stl_queue<Impl>(state, max_n, accesses);
        } else {
            static_assert_unreachable<Impl>();
        }
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(accesses) * 2);
    state.SetLabel(Impl::impl_name);
}

#define BENCH(...)                                                                                \
    BENCHMARK_TEMPLATE(access_ends, __VA_ARGS__)->Apply(range::exponential<65536>)

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
