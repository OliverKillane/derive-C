/// @brief Benchmark sequential allocations of various sizes
///  - Tests allocator performance for simple allocation patterns
///  - Compares stdalloc vs hybridstatic with different buffer sizes

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

template <typename AllocImpl>
void sequential_alloc_case(benchmark::State& /* state */, size_t num_allocs, size_t alloc_size) {
    std::vector<void*> ptrs;
    ptrs.reserve(num_allocs);
    
    if constexpr (LABEL_CHECK(AllocImpl, stdalloc)) {
        // Allocate sequentially
        for (size_t i = 0; i < num_allocs; i++) {
            void* ptr = stdalloc_allocate_uninit(stdalloc_get_ref(), alloc_size);
            ptrs.push_back(ptr);
        }
        
        // Deallocate in same order
        for (size_t i = 0; i < num_allocs; i++) {
            stdalloc_deallocate(stdalloc_get_ref(), ptrs[i], alloc_size);
        }
    } else {
        // HybridStatic allocators
        typename AllocImpl::Alloc_buffer buffer = {};
        typename AllocImpl::Alloc alloc = AllocImpl::Alloc_new(&buffer, stdalloc_get_ref());
        
        // Allocate sequentially
        for (size_t i = 0; i < num_allocs; i++) {
            void* ptr = AllocImpl::Alloc_allocate_uninit(&alloc, alloc_size);
            ptrs.push_back(ptr);
        }
        
        // Deallocate in same order
        for (size_t i = 0; i < num_allocs; i++) {
            AllocImpl::Alloc_deallocate(&alloc, ptrs[i], alloc_size);
        }
    }
}

template <typename Impl>
void sequential_alloc(benchmark::State& state) {
    const std::size_t num_allocs = static_cast<std::size_t>(state.range(0));
    const std::size_t alloc_size = static_cast<std::size_t>(state.range(1));

    for (auto _ : state) {
        sequential_alloc_case<Impl>(state, num_allocs, alloc_size);
    }
    
    // Report throughput
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(num_allocs));
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(num_allocs) * static_cast<int64_t>(alloc_size));
}

// Benchmark with different allocation counts and sizes
#define BENCH(IMPL)                                                                            \
    BENCHMARK_TEMPLATE(sequential_alloc, IMPL)->Args({8, 16});                                \
    BENCHMARK_TEMPLATE(sequential_alloc, IMPL)->Args({8, 64});                                \
    BENCHMARK_TEMPLATE(sequential_alloc, IMPL)->Args({8, 256});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, IMPL)->Args({64, 16});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, IMPL)->Args({64, 64});                               \
    BENCHMARK_TEMPLATE(sequential_alloc, IMPL)->Args({64, 256});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, IMPL)->Args({256, 16});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, IMPL)->Args({256, 64});                              \
    BENCHMARK_TEMPLATE(sequential_alloc, IMPL)->Args({256, 256})

BENCH(StdAlloc);
BENCH(HybridStatic1K);
BENCH(HybridStatic4K);
BENCH(HybridStatic16K);

#undef BENCH
