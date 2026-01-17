/// @brief Benchmark mixed allocation sizes
///  - Tests allocator performance with varying allocation sizes
///  - Simulates realistic workloads with different object sizes

#pragma once

#include <benchmark/benchmark.h>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include "../instances.hpp"
#include "../../utils/seed.hpp"
#include "../../utils/generator.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#include <derive-cpp/meta/labels.hpp>

// Allocation size pattern: small, medium, large cycling
static constexpr size_t SMALL_SIZE = 16;
static constexpr size_t MEDIUM_SIZE = 128;
static constexpr size_t LARGE_SIZE = 512;

template <typename AllocImpl>
void mixed_sizes_case(benchmark::State& /* state */, size_t num_allocs) {
    std::vector<void*> ptrs;
    std::vector<size_t> sizes;
    ptrs.reserve(num_allocs);
    sizes.reserve(num_allocs);
    
    if constexpr (LABEL_CHECK(AllocImpl, stdalloc)) {
        // Allocate with mixed sizes
        for (size_t i = 0; i < num_allocs; i++) {
            size_t size;
            if (i % 3 == 0) {
                size = SMALL_SIZE;
            } else if (i % 3 == 1) {
                size = MEDIUM_SIZE;
            } else {
                size = LARGE_SIZE;
            }
            sizes.push_back(size);
            
            void* ptr = stdalloc_allocate_uninit(stdalloc_get_ref(), size);
            ptrs.push_back(ptr);
        }
        
        // Deallocate in reverse order (stress allocator)
        for (size_t i = num_allocs; i > 0; i--) {
            size_t idx = i - 1;
            stdalloc_deallocate(stdalloc_get_ref(), ptrs[idx], sizes[idx]);
        }
    } else {
        // HybridStatic allocators
        typename AllocImpl::Alloc_buffer buffer = {};
        typename AllocImpl::Alloc alloc = AllocImpl::Alloc_new(&buffer, stdalloc_get_ref());
        
        // Allocate with mixed sizes
        for (size_t i = 0; i < num_allocs; i++) {
            size_t size;
            if (i % 3 == 0) {
                size = SMALL_SIZE;
            } else if (i % 3 == 1) {
                size = MEDIUM_SIZE;
            } else {
                size = LARGE_SIZE;
            }
            sizes.push_back(size);
            
            void* ptr = AllocImpl::Alloc_allocate_uninit(&alloc, size);
            ptrs.push_back(ptr);
        }
        
        // Deallocate in reverse order (stress allocator)
        for (size_t i = num_allocs; i > 0; i--) {
            size_t idx = i - 1;
            AllocImpl::Alloc_deallocate(&alloc, ptrs[idx], sizes[idx]);
        }
    }
}

template <typename Impl>
void mixed_sizes(benchmark::State& state) {
    const std::size_t num_allocs = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        mixed_sizes_case<Impl>(state, num_allocs);
    }
    
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(num_allocs));
}

#define BENCH(IMPL)                                                                            \
    BENCHMARK_TEMPLATE(mixed_sizes, IMPL)->Range(8, 256)

BENCH(StdAlloc);
BENCH(HybridStatic1K);
BENCH(HybridStatic4K);
BENCH(HybridStatic16K);

#undef BENCH
