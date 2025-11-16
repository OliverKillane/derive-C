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

#define TRAIT_DEBUGABLE(SELF)                                                                      \
    REQUIRE_METHOD(void, SELF, debug, (SELF const*, debug_fmt fmt, FILE*));

#define NO_DEBUG PRIV(no_debug)

static void PRIV(no_debug)(void const* self, debug_fmt fmt, FILE* stream) {
    (void)self;
    (void)fmt;
    fprintf(stream, "(no debug provided)");
}

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

static void string_debug(char const* const* string, debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, "char*@%p \"%s\"", *string, *string);
}

#if defined __cplusplus
    #include <type_traits>

    #define _DEFAULT_DEBUG_CASE(TYPE, _, FMT, STREAM)                                              \
        if constexpr (std::is_same_v<                                                              \
                          TYPE, std::remove_cv_t<std::remove_reference_t<decltype(*item)>>>) {     \
            NS(TYPE, debug)(item, FMT, STREAM);                                                    \
        } else

    #define _DEFAULT_DEBUG(SELF, FMT, STREAM)                                                      \
        [&]<typename T>(T item) {                                                                  \
            STD_REFLECT(_DEFAULT_DEBUG_CASE, FMT, STREAM)                                          \
            if constexpr (std::is_same_v<char*, std::remove_cv_t<                                  \
                                                    std::remove_reference_t<decltype(*item)>>>) {  \
                string_debug(item, FMT, STREAM);                                                   \
            } else {                                                                               \
                NO_DEBUG(item, FMT, STREAM);                                                       \
            }                                                                                      \
        }(SELF)

#else
    #define _DEFAULT_DEBUG_CASE(TYPE, _, x)                                                        \
    TYPE:                                                                                          \
        NS(TYPE, debug),

    #define _DEFAULT_DEBUG(SELF, FMT, STREAM)                                                      \
        _Generic(*(SELF),                                                                          \
            STD_REFLECT(_DEFAULT_DEBUG_CASE, f) char const*: string_debug,                         \
            default: PRIV(no_debug))(SELF, FMT, STREAM);
#endif

#define DEFAULT_DEBUG _DEFAULT_DEBUG
