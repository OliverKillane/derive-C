/// @file mixed_sizes.hpp
/// @brief Variable-sized allocations
///
/// Checking Regressions For:
/// - Size-class selection logic
/// - Internal fragmentation across size classes
/// - Reverse-order deallocation (LIFO pattern)
/// - Slab allocators with fixed slots handling variable requests
/// - HybridStatic buffer thrashing with non-uniform sizes
///
/// Representative:
/// Not production representative. Deterministic 3-size cycle is artificial,
/// while real workloads have less predictable size distributions.

#pragma once

#include <benchmark/benchmark.h>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include "../instances.hpp"
#include "../../utils/seed.hpp"
#include "../../utils/generator.hpp"
#include "../../utils/range.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#include <derive-cpp/meta/labels.hpp>
#include <derive-cpp/meta/unreachable.hpp>

static constexpr size_t SMALL_SIZE = 16;
static constexpr size_t MEDIUM_SIZE = 128;
static constexpr size_t LARGE_SIZE = 512;

void mixed_sizes_case_stdalloc(benchmark::State& /* state */, size_t num_allocs) {
    std::vector<void*> ptrs;
    std::vector<size_t> sizes;
    ptrs.reserve(num_allocs);
    sizes.reserve(num_allocs);
    
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
    
    for (size_t i = num_allocs; i > 0; i--) {
        size_t idx = i - 1;
        stdalloc_deallocate(stdalloc_get_ref(), ptrs[idx], sizes[idx]);
    }
}

template <typename AllocImpl>
void mixed_sizes_case_chunkedbump_or_slab(benchmark::State& /* state */, size_t num_allocs) {
    std::vector<void*> ptrs;
    std::vector<size_t> sizes;
    ptrs.reserve(num_allocs);
    sizes.reserve(num_allocs);
    
    typename AllocImpl::Alloc alloc = AllocImpl::Alloc_new(stdalloc_get_ref());
    
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
    
    for (size_t i = num_allocs; i > 0; i--) {
        size_t idx = i - 1;
        AllocImpl::Alloc_deallocate(&alloc, ptrs[idx], sizes[idx]);
    }
    
    AllocImpl::Alloc_delete(&alloc);
}

template <typename AllocImpl>
void mixed_sizes_case_hybridstatic(benchmark::State& /* state */, size_t num_allocs) {
    std::vector<void*> ptrs;
    std::vector<size_t> sizes;
    ptrs.reserve(num_allocs);
    sizes.reserve(num_allocs);
    
    typename AllocImpl::Alloc_buffer buffer = {};
    typename AllocImpl::Alloc alloc = AllocImpl::Alloc_new(&buffer, stdalloc_get_ref());
    
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
    
    for (size_t i = num_allocs; i > 0; i--) {
        size_t idx = i - 1;
        AllocImpl::Alloc_deallocate(&alloc, ptrs[idx], sizes[idx]);
    }
}

template <typename Impl>
void mixed_sizes(benchmark::State& state) {
    const std::size_t num_allocs = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        if constexpr (LABEL_CHECK(Impl, stdalloc)) {
            mixed_sizes_case_stdalloc(state, num_allocs);
        } else if constexpr (LABEL_CHECK(Impl, chunkedbump) || LABEL_CHECK(Impl, slab)) {
            mixed_sizes_case_chunkedbump_or_slab<Impl>(state, num_allocs);
        } else if constexpr (LABEL_CHECK(Impl, hybridstatic)) {
            mixed_sizes_case_hybridstatic<Impl>(state, num_allocs);
        } else {
            static_assert_unreachable<Impl>();
        }
    }
    
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(num_allocs));
}

#define BENCH(...)                                                                            \
    BENCHMARK_TEMPLATE(mixed_sizes, __VA_ARGS__)->Apply(range::exponential<8192>)

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
