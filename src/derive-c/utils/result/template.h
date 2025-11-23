/// @brief A simple result type, using the (already) optional pointer type
// for access  to errors or successes

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/self/def.h>

#if !defined OK
    #if !defined PLACEHOLDERS
TEMPLATE_ERROR("No OK")
    #endif
    #define OK ok_t
typedef struct {
    int x;
} OK;
#endif

#if !defined OK_DELETE
    #define OK_DELETE NO_DELETE
#endif

#if !defined OK_EQ
    #define OK_EQ MEM_EQ
#endif

#if !defined OK_CLONE
    #define OK_CLONE COPY_CLONE
#endif

#if !defined OK_DEBUG
    #define OK_DEBUG DEFAULT_DEBUG
#endif

#if !defined ERROR
    #if !defined PLACEHOLDERS
TEMPLATE_ERROR("No ERROR")
    #endif
    #define ERROR error_t
typedef struct {
    int x;
} ERROR;
#endif

#if !defined ERROR_THROW
    #define ERROR_THROW(_) PANIC("Unexpected error in " EXPAND_STRING(SELF))
#endif

#if !defined ERROR_DELETE
    #define ERROR_DELETE NO_DELETE
#endif

#if !defined ERROR_EQ
    #define ERROR_EQ MEM_EQ
#endif

#if !defined ERROR_CLONE
    #define ERROR_CLONE COPY_CLONE
#endif

#if !defined ERROR_DEBUG
    #define ERROR_DEBUG DEFAULT_DEBUG
#endif

typedef struct {
    bool success;
    union {
        OK ok;
        ERROR error;
    } result;
} SELF;

static SELF NS(SELF, from_ok)(OK value) {
    return (SELF){
        .success = true,
        .result =
            {
                .ok = value,
            },
    };
}

static SELF NS(SELF, from_error)(ERROR value) {
    return (SELF){
        .success = false,
        .result =
            {
                .error = value,
            },
    };
}

static bool NS(SELF, is_error)(SELF const* self) { return !self->success; }

static OK const* NS(SELF, strict_get_const)(SELF const* self) {
    if (!self->success) {
        ERROR_THROW(&self->result.error);
    }
    return &self->result.ok;
}

static OK const* NS(SELF, get_okay)(SELF const* self) {
    if (self->success) {
        return &self->result.ok;
    }
    return NULL;
}

static ERROR const* NS(SELF, get_error)(SELF const* self) {
    if (!self->success) {
        return &self->result.error;
    }
    return NULL;
}

static void NS(SELF, debug)(SELF const* self, debug_fmt fmt, FILE* stream) {
    fprintf(stream, EXPAND_STRING(SELF) "@%p {\n", self);
    fmt = debug_fmt_scope_begin(fmt);
    if (self->success) {
        debug_fmt_print(fmt, stream, "ok: ");
        OK_DEBUG(&self->result.ok, fmt, stream);
        fprintf(stream, ",\n");
    } else {
        debug_fmt_print(fmt, stream, "error: ");
        ERROR_DEBUG(&self->result.error, fmt, stream);
        fprintf(stream, ",\n");
    }
    fmt = debug_fmt_scope_end(fmt);
    debug_fmt_print(fmt, stream, "}");
}

static void NS(SELF, delete)(SELF* self) {
    if (self->success) {
        OK_DELETE(&self->result.ok);
    } else {
        ERROR_DELETE(&self->result.error);
    }
}

#undef OK
#undef OK_DELETE
#undef OK_EQ
#undef OK_CLONE
#undef OK_DEBUG
#undef OK_DELETE
#undef ERROR
#undef ERROR_THROW
#undef ERROR_DELETE
#undef ERROR_EQ
#undef ERROR_CLONE
#undef ERROR_DEBUG

#include <derive-c/core/includes/undef.h>
#include <derive-c/core/self/undef.h>