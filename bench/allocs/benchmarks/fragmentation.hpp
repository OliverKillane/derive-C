/// @brief Benchmark allocator behavior under fragmentation
///  - Tests performance with interleaved alloc/dealloc patterns
///  - Simulates fragmentation scenarios

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
void fragmentation_case(benchmark::State& /* state */, size_t num_allocs, size_t alloc_size) {
    std::vector<void*> ptrs;
    ptrs.reserve(num_allocs);
    
    if constexpr (LABEL_CHECK(AllocImpl, stdalloc)) {
        // Phase 1: Allocate all
        for (size_t i = 0; i < num_allocs; i++) {
            void* ptr = stdalloc_allocate_uninit(stdalloc_get_ref(), alloc_size);
            ptrs.push_back(ptr);
        }
        
        // Phase 2: Free every other allocation (create holes)
        for (size_t i = 0; i < num_allocs; i += 2) {
            stdalloc_deallocate(stdalloc_get_ref(), ptrs[i], alloc_size);
            ptrs[i] = nullptr;
        }
        
        // Phase 3: Try to fill holes with new allocations
        for (size_t i = 0; i < num_allocs; i += 2) {
            ptrs[i] = stdalloc_allocate_uninit(stdalloc_get_ref(), alloc_size);
        }
        
        // Phase 4: Clean up all remaining allocations
        for (size_t i = 0; i < num_allocs; i++) {
            if (ptrs[i] != nullptr) {
                stdalloc_deallocate(stdalloc_get_ref(), ptrs[i], alloc_size);
            }
        }
    } else {
        // HybridStatic allocators
        typename AllocImpl::Alloc_buffer buffer = {};
        typename AllocImpl::Alloc alloc = AllocImpl::Alloc_new(&buffer, stdalloc_get_ref());
        
        // Phase 1: Allocate all
        for (size_t i = 0; i < num_allocs; i++) {
            void* ptr = AllocImpl::Alloc_allocate_uninit(&alloc, alloc_size);
            ptrs.push_back(ptr);
        }
        
        // Phase 2: Free every other allocation (create holes)
        for (size_t i = 0; i < num_allocs; i += 2) {
            AllocImpl::Alloc_deallocate(&alloc, ptrs[i], alloc_size);
            ptrs[i] = nullptr;
        }
        
        // Phase 3: Try to fill holes with new allocations
        for (size_t i = 0; i < num_allocs; i += 2) {
            ptrs[i] = AllocImpl::Alloc_allocate_uninit(&alloc, alloc_size);
        }
        
        // Phase 4: Clean up all remaining allocations
        for (size_t i = 0; i < num_allocs; i++) {
            if (ptrs[i] != nullptr) {
                AllocImpl::Alloc_deallocate(&alloc, ptrs[i], alloc_size);
            }
        }
    }
}

template <typename Impl>
void fragmentation(benchmark::State& state) {
    const std::size_t num_allocs = static_cast<std::size_t>(state.range(0));
    const std::size_t alloc_size = static_cast<std::size_t>(state.range(1));

    for (auto _ : state) {
        fragmentation_case<Impl>(state, num_allocs, alloc_size);
    }
    
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(num_allocs) * 2); // 2x for realloc phase
}

#define BENCH(IMPL)                                                                            \
    BENCHMARK_TEMPLATE(fragmentation, IMPL)->Args({32, 32});                                  \
    BENCHMARK_TEMPLATE(fragmentation, IMPL)->Args({32, 128});                                 \
    BENCHMARK_TEMPLATE(fragmentation, IMPL)->Args({128, 32});                                 \
    BENCHMARK_TEMPLATE(fragmentation, IMPL)->Args({128, 128})

BENCH(StdAlloc);
BENCH(HybridStatic1K);
BENCH(HybridStatic4K);
BENCH(HybridStatic16K);

#undef BENCH
