/// @brief A simple optional type, using the (already) optional pointer type
// for access

#include <stdbool.h>
#include <stddef.h>

#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>

#include <derive-c/core/self/def.h>

#ifndef T
    #ifndef __clang_analyzer__
        #error "The contained type must be defined"
    #endif
typedef struct {
    int x;
} derive_c_parameter_t;
    #define T derive_c_parameter_t
static void derive_c_parameter_t_delete(derive_c_parameter_t* UNUSED(self)) {}
    #define T_DELETE derive_c_parameter_t_delete
#endif

#ifndef T_DELETE
    #define T_DELETE(value)
#endif

typedef struct {
    union {
        T value;
    };
    bool present;
    gdb_marker derive_c_option;
} SELF;

static SELF NS(SELF, from)(T value) { return (SELF){.value = value, .present = true}; }

static SELF NS(SELF, empty)() { return (SELF){.present = false}; }

static T* NS(SELF, get)(SELF* self) {
    DEBUG_ASSERT(self);
    if (self->present) {
        return &self->value;
    }
    return NULL;
}

static T const* NS(SELF, get_const)(SELF const* self) {
    DEBUG_ASSERT(self);
    if (self->present) {
        return &self->value;
    }
    return NULL;
}

static bool NS(SELF, is_present)(SELF const* self) {
    DEBUG_ASSERT(self);
    return self->present;
}

static void NS(SELF, delete)(SELF* self) {
    DEBUG_ASSERT(self);
    if (self->present) {
        T_DELETE(&self->value);
    }
}

static bool NS(SELF, replace)(SELF* self, T value) {
    DEBUG_ASSERT(self);
    bool was_present;
    if (self->present) {
        T_DELETE(&self->value);
        was_present = true;
    } else {
        was_present = false;
    }
    self->value = value;
    self->present = true;
    return was_present;
}

#undef T
#undef T_DELETE

#include <derive-c/core/self/undef.h>
