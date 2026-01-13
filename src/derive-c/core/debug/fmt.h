/// @brief Debug format helpers for debug printin data structures.
#pragma once

#include <derive-c/core/panic.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    size_t indent;
} dc_debug_fmt;

static dc_debug_fmt dc_debug_fmt_new() {
    dc_debug_fmt fmt = {.indent = 0};
    return fmt;
}

static void dc_debug_fmt_print_indents(dc_debug_fmt fmt, FILE* stream) {
    for (size_t i = 0; i < fmt.indent; i++) {
        fprintf(stream, "  ");
    }
}

static void dc_debug_fmt_print(dc_debug_fmt fmt, FILE* stream, const char* format, ...)
#if defined(__clang__) || defined(__GNUC__)
    __attribute__((format(printf, 3, 4)))
#endif
    ;

static void dc_debug_fmt_print(dc_debug_fmt fmt, FILE* stream, const char* format, ...) {
    dc_debug_fmt_print_indents(fmt, stream);

    va_list args;
    va_start(args, format);
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
    vfprintf(stream, format, args);
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif
    va_end(args);
}

/// Starts a scope `{ ... }`
/// - Does not prepend with the indent
static inline dc_debug_fmt dc_debug_fmt_scope_begin(dc_debug_fmt fmt) {
    dc_debug_fmt next = {.indent = fmt.indent + 1};
    return next;
}

/// Ends a scope `{ ... }`
/// - prepends with the fmt specified indent
static inline dc_debug_fmt dc_debug_fmt_scope_end(dc_debug_fmt fmt) {
    DC_ASSERT(fmt.indent > 0);
    dc_debug_fmt next = {.indent = fmt.indent - 1};
    return next;
}
