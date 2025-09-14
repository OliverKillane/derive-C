/// @brief A vector storing the first N elements in-place, and optionally spilling additional
/// elements to a heap vector.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>

#include <derive-c/core/self/def.h>

#ifndef T
    #ifndef __clang_analyzer__
        #error "The contained type must be defined for a vector template"
    #endif
typedef struct {
    int x;
} derive_c_parameter_t;
    #define T derive_c_parameter_t // Allows independent debugging
static void derive_c_parameter_t_delete(derive_c_parameter_t* UNUSED(self)) {}
    #define T_DELETE derive_c_parameter_t_delete
static derive_c_parameter_t derive_c_parameter_t_clone(derive_c_parameter_t const* i) { return *i; }
    #define T_CLONE derive_c_parameter_t_clone
#endif

#ifndef T_DELETE
    #define T_DELETE(value)
#endif

#if !defined T_CLONE
    #define T_CLONE(value) (*(value))
#endif

#ifndef INPLACE_CAPACITY
    #ifndef __clang_analyzer__
        #error "The number of elements to store in-place must be defined"
    #endif
    #define INPLACE_CAPACITY 8
#endif

#if INPLACE_CAPACITY <= 255
    #define INPLACE_TYPE uint8_t
#elif INPLACE_CAPACITY <= 65535
    #define INPLACE_TYPE uint16_t
#else
    #error "INPLACE_CAPACITY must be less than or equal to 65535"
    #define INPLACE_TYPE size_t
#endif

typedef struct {
    INPLACE_TYPE size;
    T data[INPLACE_CAPACITY];
    gdb_marker derive_c_staticvec;
} SELF;

static SELF NS(SELF, new)() { return (SELF){.size = 0}; }

static const INPLACE_TYPE NS(SELF, max_size) = INPLACE_CAPACITY;

static SELF NS(SELF, clone)(SELF const* self) {
    SELF new_self = NS(SELF, new)();
    new_self.size = self->size;
    for (INPLACE_TYPE i = 0; i < self->size; i++) {
        new_self.data[i] = T_CLONE(&self->data[i]);
    }
    return new_self;
}

static T const* NS(SELF, try_read)(SELF const* self, INPLACE_TYPE index) {
    DEBUG_ASSERT(self);
    if (LIKELY(index < self->size)) {
        return &self->data[index];
    }
    return NULL;
}

static T const* NS(SELF, read)(SELF const* self, INPLACE_TYPE index) {
    T const* value = NS(SELF, try_read)(self, index);
    ASSERT(value);
    return value;
}

static T* NS(SELF, try_write)(SELF* self, INPLACE_TYPE index) {
    DEBUG_ASSERT(self);
    if (LIKELY(index < self->size)) {
        return &self->data[index];
    }
    return NULL;
}

static T* NS(SELF, write)(SELF* self, INPLACE_TYPE index) {
    T* value = NS(SELF, try_write)(self, index);
    ASSERT(value);
    return value;
}

static T* NS(SELF, try_push)(SELF* self, T value) {
    DEBUG_ASSERT(self);
    if (self->size < INPLACE_CAPACITY) {
        T* slot = &self->data[self->size];
        *slot = value;
        self->size++;
        return slot;
    }
    return NULL;
}

static bool NS(SELF, try_insert_at)(SELF* self, size_t at, T const* data, size_t items) {
    DEBUG_ASSERT(self);
    DEBUG_ASSERT(data);
    ASSERT(at <= self->size);

    if (self->size + items > INPLACE_CAPACITY) {
        return false;
    }

    memmove(&self->data[at + items], &self->data[at], (self->size - at) * sizeof(T));
    memcpy(&self->data[at], data, items * sizeof(T));
    self->size += items;
    return true;
}

static void NS(SELF, remove_at)(SELF* self, size_t at, size_t items) {
    DEBUG_ASSERT(self);
    ASSERT(at + items <= self->size);

    if (items == 0) {
        return;
    }

    for (size_t i = at; i < at + items; i++) {
        T_DELETE(&self->data[i]);
    }

    memmove(&self->data[at], &self->data[at + items], (self->size - (at + items)) * sizeof(T));
    self->size -= items;
}

static T* NS(SELF, push)(SELF* self, T value) {
    T* slot = NS(SELF, try_push)(self, value);
    ASSERT(slot);
    return slot;
}

static bool NS(SELF, try_pop)(SELF* self, T* destination) {
    DEBUG_ASSERT(self);
    if (LIKELY(self->size > 0)) {
        self->size--;
        *destination = self->data[self->size];
        return true;
    }
    return false;
}

static T NS(SELF, pop)(SELF* self) {
    T entry;
    ASSERT(NS(SELF, try_pop)(self, &entry));
    return entry;
}

static size_t NS(SELF, size)(SELF const* self) {
    DEBUG_ASSERT(self);
    return self->size;
}

static void NS(SELF, delete)(SELF* self) {
    DEBUG_ASSERT(self);
    for (INPLACE_TYPE i = 0; i < self->size; i++) {
        T_DELETE(&self->data[i]);
    }
}

#define ITER NS(SELF, iter)

typedef struct {
    SELF* vec;
    size_t pos;
} ITER;

static T* NS(ITER, next)(ITER* iter) {
    DEBUG_ASSERT(iter);
    if (iter->pos < iter->vec->size) {
        T* item = &iter->vec->data[iter->pos];
        iter->pos++;
        return item;
    }
    return NULL;
}

static size_t NS(ITER, position)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos;
}

static bool NS(ITER, empty)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos >= iter->vec->size;
}

static ITER NS(SELF, get_iter)(SELF* self) {
    DEBUG_ASSERT(self);
    return (ITER){
        .vec = self,
        .pos = 0,
    };
}

#undef ITER

#define ITER_CONST NS(SELF, iter_const)

typedef struct {
    SELF const* vec;
    size_t pos;
} ITER_CONST;

static T const* NS(ITER_CONST, next)(ITER_CONST* iter) {
    DEBUG_ASSERT(iter);
    if (iter->pos < iter->vec->size) {
        T const* item = &iter->vec->data[iter->pos];
        iter->pos++;
        return item;
    }
    return NULL;
}

static size_t NS(ITER_CONST, position)(ITER_CONST const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos;
}

static bool NS(ITER_CONST, empty)(ITER_CONST const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos >= iter->vec->size;
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    DEBUG_ASSERT(self);
    return (ITER_CONST){
        .vec = self,
        .pos = 0,
    };
}

#undef ITER_CONST
#undef INPLACE_TYPE

#undef T
#undef T_DELETE
#undef INPLACE_CAPACITY

#include <derive-c/core/self/undef.h>
