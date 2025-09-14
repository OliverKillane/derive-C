// JUSTIFY: No guards, just for each macro
//           - allows overriding panic, differently for each template instantiation
#include <assert.h>

#if !defined PANIC
    #include <stdio.h> // NOLINT(misc-include-cleaner) (for default panic implementation)
    #define PANIC(...)                                                                             \
        do {                                                                                       \
            fprintf(stderr, __VA_ARGS__);                                                          \
            abort();                                                                               \
        } while (0);
#endif

#if !defined ASSERT
    #define ASSERT(expr, ...)                                                                      \
        if (!(expr)) {                                                                             \
            PANIC("assertion " #expr " failed: " __VA_ARGS__ "\n");                                \
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

#if !defined NDEBUG
    #define DEBUG_ASSERT(expr) ASSERT(expr)
#else
    #define DEBUG_ASSERT(expr)
#endif