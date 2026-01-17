/// @brief Benchmark zeroed allocations (calloc vs malloc+memset behavior)
///  - Tests allocate_zeroed performance
///  - Compares different allocation sizes

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
void zeroed_alloc_case(benchmark::State& /* state */, size_t num_allocs, size_t alloc_size) {
    std::vector<void*> ptrs;
    ptrs.reserve(num_allocs);
    
    if constexpr (LABEL_CHECK(AllocImpl, stdalloc)) {
        // Allocate zeroed memory
        for (size_t i = 0; i < num_allocs; i++) {
            void* ptr = stdalloc_allocate_zeroed(stdalloc_get_ref(), alloc_size);
            ptrs.push_back(ptr);
            benchmark::DoNotOptimize(ptr);
        }
        
        // Deallocate
        for (size_t i = 0; i < num_allocs; i++) {
            stdalloc_deallocate(stdalloc_get_ref(), ptrs[i], alloc_size);
        }
    } else {
        // HybridStatic allocators
        typename AllocImpl::Alloc_buffer buffer = {};
        typename AllocImpl::Alloc alloc = AllocImpl::Alloc_new(&buffer, stdalloc_get_ref());
        
        // Allocate zeroed memory
        for (size_t i = 0; i < num_allocs; i++) {
            void* ptr = AllocImpl::Alloc_allocate_zeroed(&alloc, alloc_size);
            ptrs.push_back(ptr);
            benchmark::DoNotOptimize(ptr);
        }
        
        // Deallocate
        for (size_t i = 0; i < num_allocs; i++) {
            AllocImpl::Alloc_deallocate(&alloc, ptrs[i], alloc_size);
        }
    }
}

template <typename Impl>
void zeroed_alloc(benchmark::State& state) {
    const std::size_t num_allocs = static_cast<std::size_t>(state.range(0));
    const std::size_t alloc_size = static_cast<std::size_t>(state.range(1));

    for (auto _ : state) {
        zeroed_alloc_case<Impl>(state, num_allocs, alloc_size);
    }
    
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(num_allocs));
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(num_allocs) * static_cast<int64_t>(alloc_size));
}

#define BENCH(IMPL)                                                                            \
    BENCHMARK_TEMPLATE(zeroed_alloc, IMPL)->Args({16, 64});                                   \
    BENCHMARK_TEMPLATE(zeroed_alloc, IMPL)->Args({16, 256});                                  \
    BENCHMARK_TEMPLATE(zeroed_alloc, IMPL)->Args({16, 1024});                                 \
    BENCHMARK_TEMPLATE(zeroed_alloc, IMPL)->Args({64, 64});                                   \
    BENCHMARK_TEMPLATE(zeroed_alloc, IMPL)->Args({64, 256});                                  \
    BENCHMARK_TEMPLATE(zeroed_alloc, IMPL)->Args({64, 1024})

BENCH(StdAlloc);
BENCH(HybridStatic1K);
BENCH(HybridStatic4K);
BENCH(HybridStatic16K);

#undef BENCH
