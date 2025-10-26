/// @brief Debug format helpers for debug printin data structures.
#pragma once

#include <derive-c/core/panic.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    size_t indent;
} debug_fmt;

static debug_fmt debug_fmt_new() { return (debug_fmt){.indent = 0}; }

static void debug_fmt_print_indents(debug_fmt fmt, FILE* stream) {
    for (size_t i = 0; i < fmt.indent; i++) {
        fprintf(stream, "  ");
    }
}

static void debug_fmt_print(debug_fmt fmt, FILE* stream, const char* format, ...) {
    debug_fmt_print_indents(fmt, stream);

    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);
}

/// Starts a scope `{ ... }`
/// - Does not prepend with the indent
debug_fmt debug_fmt_scope_begin(debug_fmt fmt) { return (debug_fmt){.indent = fmt.indent + 1}; }

/// Ends a scope `{ ... }`
/// - prepends with the fmt specified indent
debug_fmt debug_fmt_scope_end(debug_fmt fmt) {
    ASSERT(fmt.indent > 0);
    return (debug_fmt){.indent = fmt.indent - 1};
}
