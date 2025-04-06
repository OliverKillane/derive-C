// ## Option
// A very simple `Option<T>` template.
 
#include <derive-c/core.h>

#include <stdbool.h>
#include <stddef.h>

#ifndef PANIC
#error "PANIC must be defined (used for unrecoverable failures)"
#define PANIC abort() // Allows independent debugging
#endif

#ifndef T
#error "T (contained type) must be defined"
typedef struct {
    int x;
} placeholder;
#define T placeholder // Allows independent debugging
#endif

#ifndef SELF
#ifndef MODULE
#error                                                                                             \
    "MODULE must be defined to use a template (it is prepended to the start of all methods, and the type)"
#endif
#define SELF NAME(MODULE, NAME(vector, T))
#endif

typedef struct {
    T value;
    bool present;
    gdb_marker derive_c_option;
} SELF;

static SELF NAME(SELF, from)(T value) { return (SELF){.value = value, .present = true}; }

static SELF NAME(SELF, empty)() { return (SELF){.present = false}; }

static T* NAME(SELF, get)(SELF* self) {
    DEBUG_ASSERT(self);
    if (self->present) {
        return &self->value;
    } else {
        return NULL;
    }
}

static T const* NAME(SELF, get_const)(SELF const* self) {
    DEBUG_ASSERT(self);
    if (self->present) {
        return &self->value;
    } else {
        return NULL;
    }
}

static bool NAME(SELF, is_present)(SELF const* self) {
    DEBUG_ASSERT(self);
    return self->present;
}

#undef SELF
#undef T
