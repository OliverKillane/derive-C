/// @file iter_mut.hpp
/// @brief Vector mutable iteration and vectorization
///
/// Checking Regressions For:
/// - Mutable iteration performance (read-modify-write)
/// - Compiler auto-vectorization effectiveness
/// - Iterator overhead vs direct indexing
/// - Element packing and alignment for small types
/// - Cache hierarchy effects on large vectors
///
/// Representative:
/// Not production representative. Trivial increment in tight loop isolates
/// iterator overhead and vectorization for simple transformations.

#pragma once

#if !defined __SSE2__
    #error "SSE2 is required for this benchmark"
#endif

#include <emmintrin.h>
#include <benchmark/benchmark.h>
#include <cstddef>

#include "../instances.hpp"
#include "../../../utils/seed.hpp"
#include "../../../utils/generator.hpp"

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#include <derive-cpp/meta/labels.hpp>
#include <derive-cpp/meta/unreachable.hpp>

struct IndexedReference {
    LABEL_ADD(indexed_reference);
    static constexpr const char* impl_name = "reference/indexed";
#define EXPAND_IN_STRUCT
#define ITEM uint8_t
#define NAME Self
#include <derive-c/container/vector/dynamic/template.h>
};

// A reference implementation (using a loop with vector ops, rather than the iterator)
struct VectorizedReference {
    LABEL_ADD(iter_mut_reference);
    static constexpr const char* impl_name = "reference/vectorized";
#define EXPAND_IN_STRUCT
#define ITEM uint8_t
#define NAME Self
#include <derive-c/container/vector/dynamic/template.h>
};

template <typename Gen>
void iter_mut_case_indexed_reference(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename IndexedReference::Self v = IndexedReference::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < max_n; i++) {
        IndexedReference::Self_push(&v, gen.next());
    }

    for (size_t i = 0; i < max_n; i++) {
        IndexedReference::Self_data(&v)[i] = IndexedReference::Self_data(&v)[i] * 2;
    }

    IndexedReference::Self_delete(&v);
}

template <typename Gen>
void iter_mut_case_iter_mut_reference(benchmark::State& /* state */, size_t size, Gen& gen) {
    typename VectorizedReference::Self v = VectorizedReference::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < size; i++) {
        VectorizedReference::Self_push(&v, gen.next());
    }

    uint8_t* data = VectorizedReference::Self_data(&v);

    size_t i = 0;
    for (; i + 16 <= size; i += 16) {
        __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + i));
        v = _mm_add_epi8(v, v); // NOLINT(portability-simd-intrinsics)
        _mm_storeu_si128(reinterpret_cast<__m128i*>(data + i), v);
    }

    for (; i < size; ++i) {
        data[i] = data[i] * 2;
    }

    VectorizedReference::Self_delete(&v);
}

template <typename NS, typename Gen>
void iter_mut_case_derive_c_dynamic(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename NS::Self v = NS::Self_new(stdalloc_get_ref());

    for (size_t i = 0; i < max_n; i++) {
        NS::Self_push(&v, gen.next());
    }

    typename NS::Self_iter iter = NS::Self_get_iter(&v);
    while (!NS::Self_iter_empty(&iter)) {
        typename NS::Self_iter_item entry = NS::Self_iter_next(&iter);
        *entry = *entry * 2;
    }

    NS::Self_delete(&v);
}

template <typename NS, typename Gen>
void iter_mut_case_derive_c_hybrid(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename NS::HybridAlloc_buffer buffer = {};
    typename NS::HybridAlloc alloc = NS::HybridAlloc_new(&buffer, stdalloc_get_ref());

    typename NS::Self v = NS::Self_new(&alloc);

    for (size_t i = 0; i < max_n; i++) {
        NS::Self_push(&v, gen.next());
    }

    typename NS::Self_iter iter = NS::Self_get_iter(&v);
    while (!NS::Self_iter_empty(&iter)) {
        typename NS::Self_iter_item entry = NS::Self_iter_next(&iter);
        *entry = *entry * 2;
    }

    NS::Self_delete(&v);
}

template <typename Std, typename Gen>
void iter_mut_case_stl(benchmark::State& /* state */, size_t max_n, Gen& gen) {
    typename Std::Self x;

    for (size_t i = 0; i < max_n; i++) {
        x.push_back(gen.next());
    }

    for (auto& entry : x) {
        entry = entry * 2;
    }
}

template <typename Impl> void iter_mut(benchmark::State& state) {
    const std::size_t max_n = static_cast<std::size_t>(state.range(0));

    U32XORShiftGen gen(SEED);

    for (auto _ : state) {
        if constexpr (LABEL_CHECK(Impl, derive_c_dynamic)) {
            iter_mut_case_derive_c_dynamic<Impl>(state, max_n, gen);
        } else if constexpr (LABEL_CHECK(Impl, derive_c_hybrid)) {
            iter_mut_case_derive_c_hybrid<Impl>(state, max_n, gen);
        } else if constexpr (LABEL_CHECK(Impl, stl_vector)) {
            iter_mut_case_stl<Impl>(state, max_n, gen);
        } else if constexpr (LABEL_CHECK(Impl, indexed_reference)) {
            iter_mut_case_indexed_reference(state, max_n, gen);
        } else if constexpr (LABEL_CHECK(Impl, iter_mut_reference)) {
            iter_mut_case_iter_mut_reference(state, max_n, gen);
        } else {
            static_assert_unreachable<Impl>();
        }
    }
    
    // Report items processed (mutations per benchmark iteration)
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(max_n));
    state.SetLabel("mutations");
}

#define BENCH(...)                                                                                \
    BENCHMARK_TEMPLATE(iter_mut, __VA_ARGS__)                                                 \
        ->RangeMultiplier(2)                                                                  \
        ->Range(1 << 10, 1 << 24)                                                             \
        ->RangeMultiplier(2)                                                                  \
        ->Range(3 << 10, 1 << 24)                                                             \
        ->RangeMultiplier(2)                                                                  \
        ->Range(5 << 10, 1 << 24)                                                             \
        ->RangeMultiplier(2)                                                                  \
        ->Range(7 << 10, 1 << 24)

BENCH(Std<uint8_t>);
BENCH(Dynamic<uint8_t>);
BENCH(Hybrid<uint8_t>);
BENCH(VectorizedReference);
BENCH(IndexedReference);

#undef BENCH