/// @brief Debug format helpers for debug printin data structures.
#pragma once

#include <derive-c/core/panic.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    size_t indent;
} dc_debug_fmt;

static dc_debug_fmt dc_debug_fmt_new() { return (dc_debug_fmt){.indent = 0}; }

static void dc_debug_fmt_print_indents(dc_debug_fmt fmt, FILE* stream) {
    for (size_t i = 0; i < fmt.indent; i++) {
        fprintf(stream, "  ");
    }
}

static void dc_debug_fmt_print(dc_debug_fmt fmt, FILE* stream, const char* format, ...) {
    dc_debug_fmt_print_indents(fmt, stream);

    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);
}

/// Starts a scope `{ ... }`
/// - Does not prepend with the indent
dc_debug_fmt dc_debug_fmt_scope_begin(dc_debug_fmt fmt) {
    return (dc_debug_fmt){.indent = fmt.indent + 1};
}

/// Ends a scope `{ ... }`
/// - prepends with the fmt specified indent
dc_debug_fmt dc_debug_fmt_scope_end(dc_debug_fmt fmt) {
    DC_ASSERT(fmt.indent > 0);
    return (dc_debug_fmt){.indent = fmt.indent - 1};
}
