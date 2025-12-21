#pragma once
#include <assert.h>

#include <derive-c/core/placeholder.h>

// JUSTIFY: Disabled under clangd to avoid spurious errors in C code.
// - We have tests & fuzz tests with panic defined using C++ headers from gtest & rapidcheck
// - Hence when analysing a header, clangd assumes this is the header as included for these tests
// - which results in errors for missing macros (e.g. RC_FAIL)
// Hence for clangd, we just assume the default implementations
#if !defined PLACEHOLDERS
    #if defined DC_PANIC_HEADER
        #include DC_PANIC_HEADER
    #endif
#endif

#if !defined DC_STATIC_ASSERT
    #if defined __cplusplus
        #define DC_STATIC_ASSERT static_assert
    #else
        #define DC_STATIC_ASSERT _Static_assert
    #endif
#endif

#if !defined DC_PANIC
    #include <stdio.h>  // NOLINT(misc-include-cleaner) (for default panic implementation)
    #include <stdlib.h> // NOLINT(misc-include-cleaner) (for default panic implementation)
    #define DC_PANIC(...)                                                                          \
        do {                                                                                       \
            fprintf(stderr, __VA_ARGS__);                                                          \
            abort();                                                                               \
        } while (0);
#endif

#if !defined DC_ASSERT
    #define DC_ASSERT(expr, ...)                                                                   \
        if (!(expr)) {                                                                             \
            DC_PANIC("assertion " #expr " failed: " __VA_ARGS__);                                  \
        }
#endif

#if !defined DC_UNREACHABLE
    #define DC_UNREACHABLE(...) DC_PANIC("unreachable: " __VA_ARGS__ "\n");
#endif

#if !defined DC_LIKELY
    #define DC_LIKELY(x) __builtin_expect(!!(x), 1)
#endif

#if !defined DC_WHEN
    #define DC_WHEN(cond, expr) ((cond) ? (expr) : true)
#endif

#if !defined DC_ASSUME
    #if !defined NDEBUG
        #define DC_ASSUME(expr, ...) DC_ASSERT(expr, __VA_ARGS__)
    #else
        #if defined(__clang__)
            #define DC_ASSUME(expr, ...) __builtin_assume(expr)
        #elif defined(__GNUC__)
            // GCC doesn't have __builtin_assume, but this pattern has the same effect:
            #define DC_ASSUME(expr, ...)                                                           \
                do {                                                                               \
                    if (!(expr))                                                                   \
                        __builtin_unreachable();                                                   \
                } while (0)
        #else
            #define DC_ASSUME(expr, ...) ((void)0)
        #endif
    #endif
#endif
