#pragma once
#include <derive-c/alloc/std.h>

#define NAME debug_string_builder
#include <derive-c/utils/string_builder/template.h>

#define DEBUG_STRING(TYPE, INSTANCE)                                                               \
    ({                                                                                             \
        debug_string_builder builder = debug_string_builder_new(stdalloc_get());                   \
        NS(TYPE, debug)(INSTANCE, dc_debug_fmt_new(), debug_string_builder_stream(&builder));      \
        char* string = debug_string_builder_release_string(&builder);                              \
        debug_string_builder_delete(&builder);                                                     \
        string;                                                                                    \
    })
