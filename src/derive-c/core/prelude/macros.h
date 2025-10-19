// JUSTIFY: No guards, just for each macro
//           - allows overriding function attributes
//           - allows overriding panic, differently for each template instantiation

#include <assert.h>

#if !defined INLINE
    #define INLINE inline __attribute__((always_inline))
#endif

#if !defined CONST
    #define CONST __attribute__((const))
#endif

#if !defined PURE
    #define PURE __attribute__((pure))
#endif

#if !defined NODISCARD
    #define NODISCARD __attribute__((warn_unused_result))
#endif

#if !defined STATIC_ASSERT
    #if defined __cplusplus
        #define STATIC_ASSERT static_assert
    #else
        #define STATIC_ASSERT _Static_assert
    #endif
#endif

#if !defined PANIC
    #include <stdio.h>  // NOLINT(misc-include-cleaner) (for default panic implementation)
    #include <stdlib.h> // NOLINT(misc-include-cleaner) (for default panic implementation)
    #define PANIC(...)                                                                             \
        do {                                                                                       \
            fprintf(stderr, __VA_ARGS__);                                                          \
            abort();                                                                               \
        } while (0);
#endif

#if !defined ASSERT
    #define ASSERT(expr, ...)                                                                      \
        if (!(expr)) {                                                                             \
            PANIC("assertion " #expr " failed: " __VA_ARGS__);                                     \
        }
#endif

#if !defined UNREACHABLE
    #define UNREACHABLE(...) PANIC("unreachable: " __VA_ARGS__ "\n");
#endif

#if !defined LIKELY
    #define LIKELY(x) __builtin_expect(!!(x), 1)
#endif

#if !defined WHEN
    #define WHEN(cond, expr) ((cond) ? (expr) : true)
#endif

#if !defined ASSUME
    #if !defined NDEBUG
        #define ASSUME(expr, ...) ASSERT(expr, __VA_ARGS__)
    #else
        #if defined(__clang__)
            #define ASSUME(expr, ...) __builtin_assume(expr)
        #elif defined(__GNUC__)
            // GCC doesn't have __builtin_assume, but this pattern has the same effect:
            #define ASSUME(expr, ...)                                                              \
                do {                                                                               \
                    if (!(expr))                                                                   \
                        __builtin_unreachable();                                                   \
                } while (0)
        #else
            #define ASSUME(expr, ...) ((void)0)
        #endif
    #endif
#endif
