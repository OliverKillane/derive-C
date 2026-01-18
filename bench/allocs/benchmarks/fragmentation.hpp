/// @file fragmentation.hpp
/// @brief Allocator fragmentation resistance
///
/// Checking Regressions For:
/// - Metadata management when deallocating every other allocation
/// - Hole-filling strategies when reallocating into freed slots
/// - Freelist traversal performance in bump allocators
/// - HybridStatic fallback-to-heap behavior under fragmentation
/// - Slab allocator slot reuse patterns
///
/// Representative:
/// Not production representative. Pathological alloc-free-alloc pattern designed
/// to stress-test internal bookkeeping rather than realistic application behavior.

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

void fragmentation_case_stdalloc(benchmark::State& /* state */, size_t num_allocs, size_t alloc_size) {
    std::vector<void*> ptrs;
    ptrs.reserve(num_allocs);
    
    for (size_t i = 0; i < num_allocs; i++) {
        void* ptr = stdalloc_allocate_uninit(stdalloc_get_ref(), alloc_size);
        ptrs.push_back(ptr);
    }
    
    for (size_t i = 0; i < num_allocs; i += 2) {
        stdalloc_deallocate(stdalloc_get_ref(), ptrs[i], alloc_size);
        ptrs[i] = nullptr;
    }
    
    for (size_t i = 0; i < num_allocs; i += 2) {
        ptrs[i] = stdalloc_allocate_uninit(stdalloc_get_ref(), alloc_size);
    }
    
    for (size_t i = 0; i < num_allocs; i++) {
        if (ptrs[i] != nullptr) {
            stdalloc_deallocate(stdalloc_get_ref(), ptrs[i], alloc_size);
        }
    }
}

template <typename AllocImpl>
void fragmentation_case_chunkedbump_or_slab(benchmark::State& /* state */, size_t num_allocs, size_t alloc_size) {
    std::vector<void*> ptrs;
    ptrs.reserve(num_allocs);
    
    typename AllocImpl::Alloc alloc = AllocImpl::Alloc_new(stdalloc_get_ref());
    
    for (size_t i = 0; i < num_allocs; i++) {
        void* ptr = AllocImpl::Alloc_allocate_uninit(&alloc, alloc_size);
        ptrs.push_back(ptr);
    }
    
    for (size_t i = 0; i < num_allocs; i += 2) {
        AllocImpl::Alloc_deallocate(&alloc, ptrs[i], alloc_size);
        ptrs[i] = nullptr;
    }
    
    for (size_t i = 0; i < num_allocs; i += 2) {
        ptrs[i] = AllocImpl::Alloc_allocate_uninit(&alloc, alloc_size);
    }
    
    for (size_t i = 0; i < num_allocs; i++) {
        if (ptrs[i] != nullptr) {
            AllocImpl::Alloc_deallocate(&alloc, ptrs[i], alloc_size);
        }
    }
    
    AllocImpl::Alloc_delete(&alloc);
}

template <typename AllocImpl>
void fragmentation_case_hybridstatic(benchmark::State& /* state */, size_t num_allocs, size_t alloc_size) {
    std::vector<void*> ptrs;
    ptrs.reserve(num_allocs);
    
    typename AllocImpl::Alloc_buffer buffer = {};
    typename AllocImpl::Alloc alloc = AllocImpl::Alloc_new(&buffer, stdalloc_get_ref());
    
    for (size_t i = 0; i < num_allocs; i++) {
        void* ptr = AllocImpl::Alloc_allocate_uninit(&alloc, alloc_size);
        ptrs.push_back(ptr);
    }
    
    for (size_t i = 0; i < num_allocs; i += 2) {
        AllocImpl::Alloc_deallocate(&alloc, ptrs[i], alloc_size);
        ptrs[i] = nullptr;
    }
    
    for (size_t i = 0; i < num_allocs; i += 2) {
        ptrs[i] = AllocImpl::Alloc_allocate_uninit(&alloc, alloc_size);
    }
    
    for (size_t i = 0; i < num_allocs; i++) {
        if (ptrs[i] != nullptr) {
            AllocImpl::Alloc_deallocate(&alloc, ptrs[i], alloc_size);
        }
    }
}

template <typename Impl>
void fragmentation(benchmark::State& state) {
    const std::size_t num_allocs = static_cast<std::size_t>(state.range(0));
    const std::size_t alloc_size = static_cast<std::size_t>(state.range(1));

    for (auto _ : state) {
        if constexpr (LABEL_CHECK(Impl, stdalloc)) {
            fragmentation_case_stdalloc(state, num_allocs, alloc_size);
        } else if constexpr (LABEL_CHECK(Impl, chunkedbump) || LABEL_CHECK(Impl, slab)) {
            fragmentation_case_chunkedbump_or_slab<Impl>(state, num_allocs, alloc_size);
        } else if constexpr (LABEL_CHECK(Impl, hybridstatic)) {
            fragmentation_case_hybridstatic<Impl>(state, num_allocs, alloc_size);
        } else {
            static_assert_unreachable<Impl>();
        }
    }
    
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(num_allocs) * 2);
}

#define BENCH(...)                                                                            \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({8, 8});                                    \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({8, 32});                                   \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({8, 128});                                  \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({12, 12});                                  \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({12, 48});                                  \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({12, 192});                                 \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({16, 16});                                  \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({16, 64});                                  \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({16, 256});                                 \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({24, 24});                                  \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({24, 96});                                  \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({24, 384});                                 \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({32, 32});                                  \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({32, 128});                                 \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({32, 512});                                 \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({48, 48});                                  \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({48, 192});                                 \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({48, 768});                                 \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({64, 32});                                  \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({64, 128});                                 \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({64, 512});                                 \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({96, 96});                                  \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({96, 384});                                 \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({96, 1536});                                \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({128, 32});                                 \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({128, 128});                                \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({128, 512});                                \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({192, 192});                                \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({192, 768});                                \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({192, 3072});                               \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({256, 64});                                 \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({256, 256});                                \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({256, 1024});                               \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({384, 384});                                \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({384, 1536});                               \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({384, 6144});                               \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({512, 128});                                \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({512, 512});                                \
    BENCHMARK_TEMPLATE(fragmentation, __VA_ARGS__)->Args({512, 2048})

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
