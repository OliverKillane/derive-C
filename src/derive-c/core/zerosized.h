#pragma once

/// Zero sized types are useful as markers (e.g. for gdb printing, or to replace debug info structs
// on release).
// - In gcc/clang C a struct with a zero sized array is itself zero sized.
// - In C++ this is not valid, and the type must be at least 1 byte. Hence for compatibility, zero
//   sized is size 1 in C++
#ifdef __cplusplus
    #define DC_ZERO_SIZED(TYPE)                                                                    \
        typedef struct {                                                                           \
            char zero_sized_marker[1];                                                             \
        } TYPE;                                                                                    \
        static_assert(sizeof(TYPE) == 1)
#else
    #define DC_ZERO_SIZED(TYPE)                                                                    \
        typedef struct {                                                                           \
            char zero_sized_marker[0];                                                             \
        } TYPE;                                                                                    \
        _Static_assert(sizeof(TYPE) == 0)
#endif
