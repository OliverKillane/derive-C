// JUSTIFY: No guards, just for each macro
//           - allows overriding panic, differently for each template instantiation
#include <assert.h>

#ifndef PANIC
    #include <stdio.h> // NOLINT(misc-include-cleaner) (for default panic implementation)
    #define PANIC(...)                                                                             \
        do {                                                                                       \
            fprintf(stderr, __VA_ARGS__);                                                          \
            abort();                                                                               \
        } while (0);
#endif

#ifndef ASSERT
    #define ASSERT(expr, ...)                                                                      \
        if (!(expr)) {                                                                             \
            PANIC("assertion " #expr " failed: " __VA_ARGS__ "\n");                                \
        }
#endif

#ifndef UNREACHABLE
    #define UNREACHABLE(...) PANIC("unreachable: " __VA_ARGS__ "\n");
#endif

#ifndef DEBUG_UNREACHABLE
    #ifndef NDEBUG
        #define DEBUG_UNREACHABLE(...) UNREACHABLE(__VA_ARGS__)
    #else
        #define DEBUG_UNREACHABLE(...) __builtin_unreachable()
    #endif
#endif

#ifndef NDEBUG
    #define DEBUG_ASSERT(expr) ASSERT(expr)
#else
    #define DEBUG_ASSERT(expr)
#endif