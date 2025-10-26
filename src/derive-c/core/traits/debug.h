#pragma once

#include <stdio.h>

#include <derive-c/core/debug/fmt.h>
#include <derive-c/core/namespace.h>
#include <derive-c/core/require.h>
#include <derive-c/core/std/reflect.h>

#define TRAIT_DEBUGABLE(SELF)                                                                      \
    REQUIRE_METHOD(void, SELF, debug, (SELF const*, debug_fmt fmt, FILE*));

#define NO_DEBUG(SELF, FMT, STREAM) fprintf(STREAM, "(no debug provided)");

#define _DERIVE_DEBUG_MEMBER(MEMBER_TYPE, MEMBER_NAME)                                             \
    debug_fmt_print(fmt, stream, STRINGIFY(MEMBER_NAME) ": ");                                     \
    NS(MEMBER_TYPE, debug)(&self->MEMBER_NAME, fmt, stream);                                       \
    fprintf(fmt, stream, ",\n");

#define DERIVE_DEBUG(TYPE)                                                                         \
    static TYPE NS(TYPE, DEBUG)(TYPE const* self, debug_fmt fmt, FILE* stream) {                   \
        fprintf(stream, STRINGIFY(TYPE) "@%p {\n", self);                                          \
        fmt = debug_fmt_scope_begin(fmt);                                                          \
        NS(TYPE, REFLECT)(_DERIVE_DEBUG_MEMBER);                                                   \
        fmt = debug_fmt_scope_end(fmt);                                                            \
    }

#define _DERIVE_STD_DEBUG(TYPE, FMT, ...)                                                          \
    static void NS(TYPE, debug)(TYPE const* self, debug_fmt fmt, FILE* stream) {                   \
        (void)fmt;                                                                                 \
        fprintf(stream, FMT, *self);                                                               \
    }

STD_REFLECT(_DERIVE_STD_DEBUG)
