#ifndef PANIC

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define PANIC(...)                                                                                 \
    do {                                                                                           \
        fprintf(stderr, __VA_ARGS__);                                                              \
        abort();                                                                                   \
    } while (0);
#endif

#ifndef ASSERT
#define ASSERT(expr, ...)                                                                          \
    if (!(expr)) {                                                                                 \
        PANIC("assertion " #expr " failed: " __VA_ARGS__ "\n");                                    \
    }
#endif

#ifdef NDEBUG
#define DEBUG_ASSERT(expr) ASSERT(expr)
#else
#define DEBUG_ASSERT(expr)
#endif
