#pragma once

#include <derive-c/core/panic.h>

/// Zero sized types are useful as markers (e.g. for gdb printing, or to replace debug info structs
// on release).
// - In gcc/clang C a struct with a zero sized array is itself zero sized.
// - In C++ standard this is not valid, but it is supported as an extension by Clang and GCC
// - A struct with an empty array is valid in both C and clang/GCC C++, an empty struct is only
// valid in C
#define DC_ZERO_SIZED(TYPE)                                                                        \
    typedef struct {                                                                               \
        char zero_sized_arr[0];                                                                    \
    } TYPE;                                                                                        \
    DC_STATIC_ASSERT(sizeof(TYPE) == 0)
