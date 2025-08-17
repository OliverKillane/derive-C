/// @brief A simple vector

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <derive-c/core.h>
#include <derive-c/panic.h>
#include <derive-c/self.h>

#ifndef ALLOC
#include <derive-c/allocs/std.h>
#define ALLOC stdalloc
#endif

#ifndef T
#ifndef __clang_analyzer__
#error "The contained type must be defined for a vector template"
#endif
typedef struct {
    int x;
} derive_c_parameter_t;
#define T derive_c_parameter_t // Allows independent debugging
static void derive_c_parameter_t_delete(derive_c_parameter_t* UNUSED(t)) {}
#define T_DELETE derive_c_parameter_t_delete
#endif

#ifndef T_DELETE
#define T_DELETE(value)
#endif

typedef struct {
    size_t size;
    size_t capacity;
    T* data;
    ALLOC* alloc;
    gdb_marker derive_c_vector;
} SELF;

static SELF NAME(SELF, new)(ALLOC* alloc) {
    SELF temp = (SELF){
        .size = 0,
        .capacity = 0,
        .data = NULL,
        .alloc = alloc,
    };
    return temp;
}

static SELF NAME(SELF, new_with_capacity)(size_t capacity, ALLOC* alloc) {
    DEBUG_ASSERT(capacity > 0);
    T* data = (T*)NAME(ALLOC, malloc)(alloc, capacity * sizeof(T));
    ASSERT(LIKELY(data));
    return (SELF){
        .size = 0,
        .capacity = capacity,
        .data = data,
        .alloc = alloc,
    };
}

static SELF NAME(SELF, new_with_defaults)(size_t size, T default_value, ALLOC* alloc) {
    T* data = (T*)NAME(ALLOC, malloc)(alloc, size * sizeof(T));
    for (size_t i = 0; i < size; i++) {
        data[i] = default_value;
    }
    ASSERT(LIKELY(data));
    return (SELF){
        .size = size,
        .capacity = size,
        .data = data,
        .alloc = alloc,
    };
}

static SELF NAME(SELF, shallow_clone)(SELF const* self) {
    DEBUG_ASSERT(self);
    T* data = (T*)NAME(ALLOC, malloc)(self->alloc, self->capacity * sizeof(T));
    ASSERT(data);
    memcpy(data, self->data, self->size * sizeof(T));
    return (SELF){
        .size = self->size,
        .capacity = self->capacity,
        .data = data,
        .alloc = self->alloc,
    };
}

static T const* NAME(SELF, try_read)(SELF const* self, size_t index) {
    DEBUG_ASSERT(self);
    if (LIKELY(index < self->size)) {
        return &self->data[index];
    }
    return NULL;
}

static T const* NAME(SELF, read)(SELF const* self, size_t index) {
    T const* value = NAME(SELF, try_read)(self, index);
    ASSERT(value);
    return value;
}

static T* NAME(SELF, try_write)(SELF* self, size_t index) {
    DEBUG_ASSERT(self);
    if (LIKELY(index < self->size)) {
        return &self->data[index];
    }
    return NULL;
}

static T* NAME(SELF, write)(SELF* self, size_t index) {
    T* value = NAME(SELF, try_write)(self, index);
    ASSERT(value);
    return value;
}

static T* NAME(SELF, push)(SELF* self, T value) {
    DEBUG_ASSERT(self);
    if (self->size == self->capacity) {
        T* new_data;
        size_t new_capacity;
        if (self->data == NULL) {
            DEBUG_ASSERT(self->capacity == 0);
            // JUSTIFY: Allocating capacity of 4
            //           - Avoid repeat reallocations on growing a vector from
            //             size sero (from new)
            //           Otherwise an arbitrary choice (given we do not know the size of T)
            new_capacity = 8;
            new_data = (T*)NAME(ALLOC, malloc)(self->alloc, new_capacity * sizeof(T));
        } else {
            // JUSTIFY: Growth factor of 2
            //           - Simple arithmetic (for debugging)
            //           - Same as used by GCC's std::vector implementation
            new_capacity = self->capacity * 2;
            new_data = (T*)NAME(ALLOC, realloc)(self->alloc, self->data, new_capacity * sizeof(T));
        }
        ASSERT(new_data);
        self->capacity = new_capacity;
        self->data = new_data;
    }
    T* entry = &self->data[self->size];
    *entry = value;
    self->size++;
    return entry;
}

static bool NAME(SELF, try_pop)(SELF* self, T* destination) {
    DEBUG_ASSERT(self);
    if (LIKELY(self->size > 0)) {
        self->size--;
        *destination = self->data[self->size];
        return true;
    }
    return false;
}

static T NAME(SELF, pop)(SELF* self) {
    T entry;
    ASSERT(NAME(SELF, try_pop)(self, &entry));
    return entry;
}

static size_t NAME(SELF, size)(SELF const* self) {
    DEBUG_ASSERT(self);
    return self->size;
}

static void NAME(SELF, delete)(SELF* self) {
    DEBUG_ASSERT(self);
    if (self->data) {
        for (size_t i = 0; i < self->size; i++) {
            T_DELETE(&self->data[i]);
        }
        NAME(ALLOC, free)(self->alloc, self->data);
    }
}

#define ITER NAME(SELF, iter)

typedef struct {
    SELF* vec;
    size_t pos;
} ITER;

static T* NAME(ITER, next)(ITER* iter) {
    DEBUG_ASSERT(iter);
    if (iter->pos < iter->vec->size) {
        T* item = &iter->vec->data[iter->pos];
        iter->pos++;
        return item;
    }
    return NULL;
}

static size_t NAME(ITER, position)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos;
}

static bool NAME(ITER, empty)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos >= iter->vec->size;
}

static ITER NAME(SELF, get_iter)(SELF* self) {
    DEBUG_ASSERT(self);
    return (ITER){
        .vec = self,
        .pos = 0,
    };
}
#undef ITER

#define ITER_CONST NAME(SELF, iter_const)

typedef struct {
    SELF const* vec;
    size_t pos;
} ITER_CONST;

static T const* NAME(ITER_CONST, next)(ITER_CONST* iter) {
    DEBUG_ASSERT(iter);
    if (iter->pos < iter->vec->size) {
        T const* item = &iter->vec->data[iter->pos];
        iter->pos++;
        return item;
    }
    return NULL;
}

static size_t NAME(ITER_CONST, position)(ITER_CONST const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos;
}

static bool NAME(ITER_CONST, empty)(ITER_CONST const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos >= iter->vec->size;
}

static ITER_CONST NAME(SELF, get_iter_const)(SELF const* self) {
    DEBUG_ASSERT(self);
    return (ITER_CONST){
        .vec = self,
        .pos = 0,
    };
}

#undef ITER_CONST

#undef SELF
#undef T
#undef T_DELETE
