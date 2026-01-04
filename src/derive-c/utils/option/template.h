/// @brief A simple optional type, using the (already) optional pointer type
// for access

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/self/def.h>

#if !defined ITEM
    #if !defined PLACEHOLDERS
        #error "The contained type must be defined"
    #endif
typedef struct {
    int x;
} item_t;
    #define ITEM item_t
    #define ITEM_DELETE item_delete
static void ITEM_DELETE(item_t* self);
    #define ITEM_CLONE item_clone
static item_t ITEM_CLONE(item_t const* self);
    #define ITEM_EQ item_eq
static bool ITEM_EQ(item_t const* a, item_t const* b);
    #define ITEM_DEBUG item_debug
static void ITEM_DEBUG(ITEM const* self, dc_debug_fmt fmt, FILE* stream);
#endif

#if !defined ITEM_DELETE
    #define ITEM_DELETE(value)
#endif

#if !defined ITEM_CLONE
    #define ITEM_CLONE(value) (*(value))
#endif

#if !defined ITEM_DEBUG
    #define ITEM_DEBUG DC_DEFAULT_DEBUG
#endif

typedef struct {
    union {
        ITEM item;
    };
    bool present;
    dc_gdb_marker derive_c_option;
} SELF;

static SELF NS(SELF, from)(ITEM value) { return (SELF){.item = value, .present = true}; }

static SELF NS(SELF, empty)() { return (SELF){.present = false}; }

static SELF NS(SELF, clone)(SELF const* self) {
    DC_ASSUME(self);
    if (self->present) {
        return NS(SELF, from)(ITEM_CLONE(&self->item));
    }
    return NS(SELF, empty)();
}

static ITEM* NS(SELF, get)(SELF* self) {
    DC_ASSUME(self);
    if (self->present) {
        return &self->item;
    }
    return NULL;
}

static ITEM const* NS(SELF, get_const)(SELF const* self) {
    DC_ASSUME(self);
    if (self->present) {
        return &self->item;
    }
    return NULL;
}

static ITEM const* NS(SELF, get_const_or)(SELF const* self, ITEM const* default_value) {
    DC_ASSUME(self);
    if (self->present) {
        return &self->item;
    }
    return default_value;
}

static ITEM NS(SELF, get_value_or)(SELF const* self, ITEM const default_value) {
    DC_ASSUME(self);
    if (self->present) {
        return self->item;
    }
    return default_value;
}

static bool NS(SELF, is_present)(SELF const* self) {
    DC_ASSUME(self);
    return self->present;
}

static void NS(SELF, delete)(SELF* self) {
    DC_ASSUME(self);
    if (self->present) {
        ITEM_DELETE(&self->item);
    }
}

static bool NS(SELF, replace)(SELF* self, ITEM value) {
    DC_ASSUME(self);
    bool was_present;
    if (self->present) {
        ITEM_DELETE(&self->item);
        was_present = true;
    } else {
        was_present = false;
    }
    self->item = value;
    self->present = true;
    return was_present;
}

static void NS(SELF, debug)(SELF* self, dc_debug_fmt fmt, FILE* stream) {
    if (self->present) {
        fprintf(stream, EXPAND_STRING(SELF) "@%p { ", self);
        fmt = dc_debug_fmt_scope_begin(fmt);
        ITEM_DEBUG(&self->item, fmt, stream);
        fprintf(stream, " }");
    } else {
        fprintf(stream, EXPAND_STRING(SELF) "@%p { NONE }", self);
    }
}

#undef ITEM_DEBUG
#undef ITEM_EQ
#undef ITEM_CLONE
#undef ITEM_DELETE
#undef ITEM

#include <derive-c/core/self/undef.h>
#include <derive-c/core/includes/undef.h>