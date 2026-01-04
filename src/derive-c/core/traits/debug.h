/// @brief The debug trait
/// Inspired by rust's `{:?}` pretty print, any data structure can be dumped to a stream.
///  - For debugging (not display) (so contains ptr information, internals)
///  - Prints the types (so users can cross reference)
///  - Print allocators (for debugging memory issues)
///  - Overrideable per data structure

#pragma once

#include <stdio.h>

#include <derive-c/core/debug/fmt.h>
#include <derive-c/core/namespace.h>
#include <derive-c/core/require.h>
#include <derive-c/core/std/reflect.h>
#include <derive-c/core/compiler.h>

#define DC_TRAIT_DEBUGABLE(SELF)                                                                   \
    DC_REQUIRE_METHOD(void, SELF, debug, (SELF const*, dc_debug_fmt fmt, FILE*));

#define NO_DEBUG PRIV(no_debug)

static void PRIV(no_debug)(void const* self, dc_debug_fmt fmt, FILE* stream) {
    (void)self;
    (void)fmt;
    fprintf(stream, "(no DEBUG function provided)");
}

#define _DC_DERIVE_DEBUG_MEMBER(MEMBER_TYPE, MEMBER_NAME)                                          \
    dc_debug_fmt_print(fmt, stream, STRINGIFY(MEMBER_NAME) ": ");                                  \
    NS(MEMBER_TYPE, debug)(&self->MEMBER_NAME, fmt, stream);                                       \
    fprintf(fmt, stream, ",\n");

#define DC_DERIVE_DEBUG(TYPE)                                                                      \
    static TYPE NS(TYPE, DEBUG)(TYPE const* self, dc_debug_fmt fmt, FILE* stream) {                \
        fprintf(stream, STRINGIFY(TYPE) "@%p {\n", self);                                          \
        fmt = dc_debug_fmt_scope_begin(fmt);                                                       \
        NS(TYPE, REFLECT)(_DC_DERIVE_DEBUG_MEMBER);                                                \
        fmt = dc_debug_fmt_scope_end(fmt);                                                         \
    }

#define _DERIVE_STD_DEBUG(TYPE, FMT, ...)                                                          \
    static void NS(TYPE, debug)(TYPE const* self, dc_debug_fmt fmt, FILE* stream) {                \
        (void)fmt;                                                                                 \
        fprintf(stream, FMT, *self);                                                               \
    }

DC_STD_REFLECT(_DERIVE_STD_DEBUG)

static void dc_string_debug(char const* const* string, dc_debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, "char*@%p \"%s\"", *string, *string);
}

#if defined DC_GENERIC_KEYWORD_SUPPORTED
    #define _DC_DEFAULT_DEBUG_CASE(TYPE, _, x)                                                     \
    TYPE:                                                                                          \
        NS(TYPE, debug),

    #define _DC_DEFAULT_DEBUG(SELF, FMT, STREAM)                                                   \
        _Generic(*(SELF),                                                                          \
            DC_STD_REFLECT(_DC_DEFAULT_DEBUG_CASE, f) char const*: dc_string_debug,                \
            default: PRIV(no_debug))(SELF, FMT, STREAM);
#else
    #include <type_traits>

    #define _DC_DEFAULT_DEBUG_CASE(TYPE, _, FMT, STREAM)                                           \
        if constexpr (std::is_same_v<                                                              \
                          TYPE, std::remove_cv_t<std::remove_reference_t<decltype(*item)>>>) {     \
            NS(TYPE, debug)(item, FMT, STREAM);                                                    \
        } else

    #define _DC_DEFAULT_DEBUG(SELF, FMT, STREAM)                                                   \
        [&]<typename T>(T item) {                                                                  \
            DC_STD_REFLECT(_DC_DEFAULT_DEBUG_CASE, FMT, STREAM)                                    \
            if constexpr (std::is_same_v<char*, std::remove_cv_t<                                  \
                                                    std::remove_reference_t<decltype(*item)>>> ||  \
                          std::is_same_v<                                                          \
                              char const*,                                                         \
                              std::remove_cv_t<std::remove_reference_t<decltype(*item)>>>) {       \
                dc_string_debug(item, FMT, STREAM);                                                \
            } else {                                                                               \
                NO_DEBUG(item, FMT, STREAM);                                                       \
            }                                                                                      \
        }(SELF)
#endif

#define DC_DEFAULT_DEBUG _DC_DEFAULT_DEBUG
