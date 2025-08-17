#pragma once
#include <stdlib.h>

#define NAME_EXPANDED(pre, post) pre##_##post
#define NAME(pre, post) NAME_EXPANDED(pre, post)

#ifndef NDEBUG
#define DEBUG_UNUSED(ident) ident __attribute__((unused))
#else
#define DEBUG_UNUSED(ident)
#endif

#define UNUSED(ident) ident __attribute__((unused))

#define LIKELY(x) __builtin_expect(!!(x), 1)

#define EXPAND(...) __VA_ARGS__

#ifdef __cplusplus
struct gdb_marker {
    char UNUSED(_dummy_cpp_object_size_compatibility);
};
#else
typedef struct {
} gdb_marker;
#endif

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

#define FORCE_INLINE inline __attribute__((always_inline))
