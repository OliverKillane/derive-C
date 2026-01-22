/// @file sequential_alloc.hpp
/// @brief Baseline allocator throughput
///
/// Checking Regressions For:
/// - Fast-path allocation (bump pointer, freelist head)
/// - FIFO deallocation overhead
/// - HybridStatic stack buffer vs heap fallback performance
/// - Slab and chunked allocator handling of uniform sizes
/// - Initialization overhead at different allocation counts
///
/// Representative:
/// Not production representative. Allocate-all-then-free-all pattern tests
/// theoretical throughput without interleaving or fragmentation.

#pragma once

#include <benchmark/benchmark.h>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include "../instances.hpp"
#include "../../utils/seed.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#include <derive-cpp/meta/labels.hpp>
#include <derive-cpp/meta/unreachable.hpp>

void sequential_alloc_case_stdalloc(benchmark::State& /* state */, size_t num_allocs, size_t alloc_size) {
    std::vector<void*> ptrs;
    ptrs.reserve(num_allocs);
    
    for (size_t i = 0; i < num_allocs; i++) {
        void* ptr = stdalloc_allocate_uninit(stdalloc_get_ref(), alloc_size);
        ptrs.push_back(ptr);
    }
    
    for (size_t i = 0; i < num_allocs; i++) {
        stdalloc_deallocate(stdalloc_get_ref(), ptrs[i], alloc_size);
    }
}

template <typename AllocImpl>
void sequential_alloc_case_chunkedbump_or_slab(benchmark::State& /* state */, size_t num_allocs, size_t alloc_size) {
    std::vector<void*> ptrs;
    ptrs.reserve(num_allocs);
    
    typename AllocImpl::Alloc alloc = AllocImpl::Alloc_new(stdalloc_get_ref());
    
    for (size_t i = 0; i < num_allocs; i++) {
        void* ptr = AllocImpl::Alloc_allocate_uninit(&alloc, alloc_size);
        ptrs.push_back(ptr);
    }
    
    for (size_t i = 0; i < num_allocs; i++) {
        AllocImpl::Alloc_deallocate(&alloc, ptrs[i], alloc_size);
    }
    
    AllocImpl::Alloc_delete(&alloc);
}

template <typename AllocImpl>
void sequential_alloc_case_hybridstatic(benchmark::State& /* state */, size_t num_allocs, size_t alloc_size) {
    std::vector<void*> ptrs;
    ptrs.reserve(num_allocs);
    
    typename AllocImpl::Alloc_buffer buffer = {};
    typename AllocImpl::Alloc alloc = AllocImpl::Alloc_new(&buffer, stdalloc_get_ref());
    
    for (size_t i = 0; i < num_allocs; i++) {
        void* ptr = AllocImpl::Alloc_allocate_uninit(&alloc, alloc_size);
        ptrs.push_back(ptr);
    }
    
    for (size_t i = 0; i < num_allocs; i++) {
        AllocImpl::Alloc_deallocate(&alloc, ptrs[i], alloc_size);
    }
}

template <typename Impl>
void sequential_alloc(benchmark::State& state) {
    const std::size_t num_allocs = static_cast<std::size_t>(state.range(0));
    const std::size_t alloc_size = static_cast<std::size_t>(state.range(1));

    for (auto _ : state) {
        if constexpr (LABEL_CHECK(Impl, stdalloc)) {
            sequential_alloc_case_stdalloc(state, num_allocs, alloc_size);
        } else if constexpr (LABEL_CHECK(Impl, chunkedbump) || LABEL_CHECK(Impl, slab)) {
            sequential_alloc_case_chunkedbump_or_slab<Impl>(state, num_allocs, alloc_size);
        } else if constexpr (LABEL_CHECK(Impl, hybridstatic)) {
            sequential_alloc_case_hybridstatic<Impl>(state, num_allocs, alloc_size);
        } else {
            static_assert_unreachable<Impl>();
        }
    }
    
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(num_allocs));
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(num_allocs) * static_cast<int64_t>(alloc_size));
}

#define BENCH(...)                                                                            \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({1, 16});                                \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({1, 64});                                \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({1, 256});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({1, 1024});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({2, 16});                                \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({2, 64});                                \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({2, 256});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({2, 1024});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({3, 16});                                \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({3, 64});                                \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({3, 256});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({3, 1024});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({4, 16});                                \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({4, 64});                                \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({4, 256});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({4, 1024});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({6, 16});                                \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({6, 64});                                \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({6, 256});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({6, 1024});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({8, 16});                                \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({8, 64});                                \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({8, 256});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({8, 1024});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({12, 16});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({12, 64});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({12, 256});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({12, 1024});                             \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({16, 16});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({16, 64});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({16, 256});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({16, 1024});                             \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({24, 16});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({24, 64});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({24, 256});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({24, 1024});                             \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({32, 16});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({32, 64});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({32, 256});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({32, 1024});                             \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({48, 16});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({48, 64});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({48, 256});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({48, 1024});                             \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({64, 16});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({64, 64});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({64, 256});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({64, 1024});                             \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({96, 16});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({96, 64});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({96, 256});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({96, 1024});                             \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({128, 16});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({128, 64});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({128, 256});                             \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({128, 1024});                            \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({192, 16});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({192, 64});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({192, 256});                             \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({192, 1024});                            \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({256, 16});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({256, 64});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({256, 256});                             \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({256, 1024});                            \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({384, 16});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({384, 64});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({384, 256});                             \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({384, 1024});                            \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({512, 16});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({512, 64});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({512, 256});                             \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({512, 1024});                            \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({768, 16});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({768, 64});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({768, 256});                             \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({768, 1024});                            \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({1024, 16});                             \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({1024, 64});                             \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({1024, 256});                            \
    BENCHMARK_TEMPLATE(sequential_alloc, __VA_ARGS__)->Args({1024, 1024})

BENCH(StdAlloc);
BENCH(HybridStatic<1024>);
BENCH(HybridStatic<4096>);
BENCH(HybridStatic<16384>);
BENCH(ChunkedBump<4096>);
BENCH(ChunkedBump<65536>);
BENCH(Slab<32, 4096>);
BENCH(Slab<64, 4096>);
BENCH(Slab<64, 65536>);

#undef BENCH
