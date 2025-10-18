/// @brief A simple optional type, using the (already) optional pointer type
// for access

#include <stdbool.h>
#include <stddef.h>

#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>
#include <derive-c/core/placeholder.h>

#include <derive-c/core/self/def.h>

#if !defined ITEM
    #if !defined PLACEHOLDERS
        #error "The contained type must be defined"
    #endif
typedef struct {
    int x;
} item_t;
    #define ITEM item_t
static void item_delete(item_t* self) { (void)self; }
    #define ITEM_DELETE item_delete
static item_t item_clone(item_t const* self) { return *self; }
    #define ITEM_CLONE item_clone
static bool item_eq(item_t const* a, item_t const* b) { return a->x == b->x; }
    #define ITEM_EQ item_eq
#endif

#if !defined ITEM_DELETE
    #define ITEM_DELETE(value)
#endif

#if !defined ITEM_CLONE
    #define ITEM_CLONE(value) (*(value))
#endif

typedef struct {
    union {
        ITEM item;
    };
    bool present;
    gdb_marker derive_c_option;
} SELF;

static SELF NS(SELF, from)(ITEM value) { return (SELF){.item = value, .present = true}; }

static SELF NS(SELF, empty)() { return (SELF){.present = false}; }

static SELF NS(SELF, clone)(SELF const* self) {
    DEBUG_ASSERT(self);
    if (self->present) {
        return NS(SELF, from)(ITEM_CLONE(&self->item));
    }
    return NS(SELF, empty)();
}

static ITEM* NS(SELF, get)(SELF* self) {
    DEBUG_ASSERT(self);
    if (self->present) {
        return &self->item;
    }
    return NULL;
}

static ITEM const* NS(SELF, get_const)(SELF const* self) {
    DEBUG_ASSERT(self);
    if (self->present) {
        return &self->item;
    }
    return NULL;
}

static ITEM const* NS(SELF, get_const_or)(SELF const* self, ITEM const* default_value) {
    DEBUG_ASSERT(self);
    if (self->present) {
        return &self->item;
    }
    return default_value;
}

static bool NS(SELF, is_present)(SELF const* self) {
    DEBUG_ASSERT(self);
    return self->present;
}

static void NS(SELF, delete)(SELF* self) {
    DEBUG_ASSERT(self);
    if (self->present) {
        ITEM_DELETE(&self->item);
    }
}

static bool NS(SELF, replace)(SELF* self, ITEM value) {
    DEBUG_ASSERT(self);
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

#undef ITEM
#undef ITEM_DELETE
#undef ITEM_CLONE

#include <derive-c/core/self/undef.h>
