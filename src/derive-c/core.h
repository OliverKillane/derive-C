#pragma once
#include <stdlib.h>

#ifndef CORE
#define CORE

#define NAME_EXPANDED(pre, post) pre##_##post
#define NAME(pre, post) NAME_EXPANDED(pre, post)

#define LIKELY(x) __builtin_expect(!!(x), 1)

#define MAYBE_NULL(T) T*
#define NEVER_NULL(T) T*

#define OUT(ptr) ptr
#define IN(ptr) ptr
#define INOUT(ptr) ptr

#define EXPAND(...) __VA_ARGS__

#define ASSERT(expr)                                                                               \
    if (!(expr))                                                                                   \
        PANIC;

typedef struct {
} gdb_marker;

#ifdef NDEBUG
#define DEBUG_ASSERT(expr) ASSERT(expr)
#else
#define DEBUG_ASSERT(expr)
#endif
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
