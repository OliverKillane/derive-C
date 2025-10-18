// JUSTIFY: No guards, just for each macro
//           - allows overriding panic, differently for each template instantiation
#include <assert.h>

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

#if !defined DEBUG_UNREACHABLE
    #if !defined NDEBUG
        #define DEBUG_UNREACHABLE(...) UNREACHABLE(__VA_ARGS__)
    #else
        #define DEBUG_UNREACHABLE(...) __builtin_unreachable()
    #endif
#endif

#if defined(__clang__)
    #define ASSUME(cond) __builtin_assume(cond)
#elif defined(_MSC_VER)
    #define ASSUME(cond) __assume(cond)
#elif defined(__GNUC__)
    // GCC doesn't have __builtin_assume, but this pattern has the same effect:
    #define ASSUME(cond)                                                                           \
        do {                                                                                       \
            if (!(cond))                                                                           \
                __builtin_unreachable();                                                           \
        } while (0)
#else
    #define ASSUME(cond) ((void)0)
#endif

#if !defined NDEBUG
    #define DEBUG_ASSERT(...) ASSERT(__VA_ARGS__)
#else
    #define DEBUG_ASSERT(expr, ...) ASSUME(expr)
#endif