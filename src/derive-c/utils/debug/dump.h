/// @brief A no-alloc, debug dump utility for use in asserts.
///  - Writes debug formatted data to a static buffer, so stable & static-lifetime
///    references can be used with string formatting.
///  - No heap allocation, so no leaking on format.

#pragma once

#include <stdio.h>
#include <string.h>
#include <derive-c/core/attributes.h>
#include <derive-c/core/namespace.h>
#include <derive-c/core/debug/fmt.h>

static const char* _dc_debug_trailing_space = "...";
#define _DC_DEBUG_MAX_CAPACITY 4096
static DC_THREAD_LOCAL char _dc_debug_dump_buf[_DC_DEBUG_MAX_CAPACITY + 4] = {};
static DC_THREAD_LOCAL size_t _dc_debug_dump_head = 0;
static DC_THREAD_LOCAL size_t _dc_debug_dump_start_pos = 0;

DC_INTERNAL static FILE* _dc_debug_dump_start(void) {
    // JUSTIFY: Single static buffer
    //  - Thread local required for thread safety
    //  - The returned strings must outlive the function call (e.g. to be used in a printf)
    //  - Multiple prints are possible.
    //  A single thread local buffer suits this task well.

    if (_dc_debug_dump_head >= _DC_DEBUG_MAX_CAPACITY) {
        return NULL;
    }

    _dc_debug_dump_start_pos = _dc_debug_dump_head;
    size_t available = _DC_DEBUG_MAX_CAPACITY - _dc_debug_dump_start_pos;
    size_t writable = available + 1;

    FILE* mem_stream = fmemopen(_dc_debug_dump_buf + _dc_debug_dump_start_pos, writable, "w");
    return mem_stream;
}

DC_INTERNAL static char const* _dc_debug_dump_end(FILE* mem_stream) {
    if (mem_stream == NULL) {
        return _dc_debug_trailing_space;
    }

    fflush(mem_stream);
    long used_long = ftell(mem_stream);
    fclose(mem_stream);

    if (used_long < 0) {
        return _dc_debug_trailing_space;
    }

    size_t used = (size_t)used_long;

    size_t start = _dc_debug_dump_start_pos;
    size_t available = _DC_DEBUG_MAX_CAPACITY - start;
    size_t trailing_len = strlen(_dc_debug_trailing_space);

    if (used > available) {
        size_t content_size = available - trailing_len;

        for (size_t i = 0; i < trailing_len; i++) {
            _dc_debug_dump_buf[start + content_size + i] = _dc_debug_trailing_space[i];
        }
        _dc_debug_dump_buf[start + content_size + trailing_len] = '\0';

        _dc_debug_dump_head = start + content_size + trailing_len + 1;
        return &_dc_debug_dump_buf[start];
    }
    _dc_debug_dump_buf[start + used] = '\0';
    _dc_debug_dump_head = start + used + 1;
    return &_dc_debug_dump_buf[start];
}

// JUSTIFY: Allowing the buffer to be reset, invalidating existing strings.
//  - Used for resetting the buffer for reuse (e.g. in unit tests)
//  - e.g. called in test constructor
DC_PUBLIC static void dc_debug_dump_reset(void) {
    _dc_debug_dump_head = 0;
    _dc_debug_dump_start_pos = 0;

    // JUSTIFY: For easier debugging in coredumps from unit tests, clear the buffer.
    memset(_dc_debug_dump_buf, 0, sizeof(_dc_debug_dump_buf));
}

// JUSTIFY: Using a macro, rather than a function
//  - We need the actual types for the debug pointer and function, we cannot convert them to
//    `void*` and `(*foo)(void*, dc_debug_fmt, FILE*)`.
//  - Additionally users are allowed to pass non-function pointers to the templates (e.g. using the
//   `DC_DEFAULT_DEBUG`, or just defining as say `#define ITEM_DEBUG(self, fmt, stream)
//   fprintf(stream, "foo")`). To avoid needing to wrap all of these in a function, we can just take
//   a macro here.
#define DC_DEBUG(DEBUG_FN, DEBUG_PTR)                                                              \
    ({                                                                                             \
        FILE* stream = _dc_debug_dump_start();                                                     \
        char const* result;                                                                        \
        if (stream == NULL) {                                                                      \
            result = _dc_debug_trailing_space;                                                     \
        } else {                                                                                   \
            DEBUG_FN(DEBUG_PTR, dc_debug_fmt_new(), stream);                                       \
            result = _dc_debug_dump_end(stream);                                                   \
        }                                                                                          \
        result;                                                                                    \
    })
