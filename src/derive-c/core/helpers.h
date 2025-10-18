#pragma once
#include <derive-c/core/panic.h>
#include <derive-c/core/zerosized.h>
#include <stdint.h>
#include <stdlib.h>

#define NS_EXPANDED(pre, post) pre##_##post
#define NS(pre, post) NS_EXPANDED(pre, post)

#define PRIVATE(name) NS(__private, name)

#if !defined NDEBUG
    #define DEBUG_UNUSED(ident) ident __attribute__((unused))
#else
    #define DEBUG_UNUSED(ident)
#endif

#define UNUSED(ident) ident __attribute__((unused))

#define LIKELY(x) __builtin_expect(!!(x), 1)

#define EXPAND(...) __VA_ARGS__

ZERO_SIZED(gdb_marker);

static gdb_marker gdb_marker_new() { return (gdb_marker){}; }

static inline size_t next_power_of_2(size_t x) {
    if (x == 0)
        return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
#if SIZE_MAX > 0xFFFFFFFF
    x |= x >> 32; // For 64-bit platforms
#endif
    return x + 1;
}

static inline bool is_power_of_2(size_t x) { return x != 0 && (x & (x - 1)) == 0; }

static inline size_t modulus_power_of_2_capacity(size_t index, size_t capacity) {
    DEBUG_ASSERT(is_power_of_2(capacity))
    // NOTE: If we know capacity is a power of 2, we can reduce the cost of 'index + 1 % capacity'
    return index & (capacity - 1);
}

static inline bool is_aligned_pow2_exp(const void* ptr, unsigned exp) {
    uintptr_t const mask = (1U << exp) - 1;
    return (((uintptr_t)ptr) & mask) == 0;
}

#define FORCE_INLINE inline __attribute__((always_inline))

#if defined __cplusplus
    #define STATIC_ASSERT static_assert
#else
    #define STATIC_ASSERT _Static_assert
#endif
