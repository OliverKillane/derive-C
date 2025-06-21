/// @brief A simple optional type, using the (already) optional pointer type 
// for access

#include <stdbool.h>
#include <stddef.h>

#include <derive-c/core.h>
#include <derive-c/panic.h>
#include <derive-c/self.h>


/// @defgroup template parameters
/// @{

#ifndef T
#error "The contained type must be defined"
typedef struct {
    int x;
} derive_c_placeholder_t;
#define T derive_c_placeholder_t
static void derive_c_placeholder_t_delete(derive_c_placeholder_t*) {}
#define T_DELETE derive_c_placeholder_t_delete
#endif

#ifndef T_DELETE
#define T_DELETE(value)
#endif

/// @}

typedef struct {
    union {
        T value;
    };
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

static T const* NAME(SELF, get_present_unsafe_unchecked)(SELF* self) {
    DEBUG_ASSERT(self);
    #ifdef NDEBUG
        T* value = NAME(SELF, get)(self);
        ASSERT(value);
        return value;
    #else
        return &self->value;
    #endif
}

static T const* NAME(SELF, get_const)(SELF const* self) {
    DEBUG_ASSERT(self);
    if (self->present) {
        return &self->value;
    } else {
        return NULL;
    }
}

static T const* NAME(SELF, get_const_present_unsafe_unchecked)(SELF const* self) {
    DEBUG_ASSERT(self);
    #ifdef NDEBUG
        T const* value = NAME(SELF, get_const)(self);
        ASSERT(value);
        return value;
    #else
        return &self->value;
    #endif
}

static bool NAME(SELF, is_present)(SELF const* self) {
    DEBUG_ASSERT(self);
    return self->present;
}

static void NAME(SELF, delete)(SELF* self) {
    DEBUG_ASSERT(self);
    if (self->present) {
        T_DELETE(&self->value);
    }
}

static bool NAME(SELF, replace)(SELF* self, T value) {
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

#undef SELF
#undef T
#undef T_DELETE
