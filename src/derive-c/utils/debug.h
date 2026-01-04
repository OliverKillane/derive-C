#pragma once
#include <derive-c/alloc/std.h>

#define NAME dc_debug_string_builder
#include <derive-c/utils/string_builder/template.h>

#define DEBUG_STRING(TYPE, INSTANCE)                                                               \
    ({                                                                                             \
        dc_debug_string_builder builder = dc_debug_string_builder_new(stdalloc_get_ref());         \
        NS(TYPE, debug)(INSTANCE, dc_debug_fmt_new(), dc_debug_string_builder_stream(&builder));   \
        char* string = dc_debug_string_builder_release_string(&builder);                           \
        dc_debug_string_builder_delete(&builder);                                                  \
        string;                                                                                    \
    })
