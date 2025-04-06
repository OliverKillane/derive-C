#include <derive-c/core.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef PANIC
#error "PANIC must be defined (used for unrecoverable failures)"
#define PANIC abort() // Allows independent debugging
#endif

#ifndef T
#error "T (contained type) must be defined for a vector template"
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
    size_t size;
    size_t capacity;
    T* data;
    gdb_marker derive_c_vector;
} SELF;

static SELF NAME(SELF, new)() {
    SELF temp = (SELF){
        .size = 0,
        .capacity = 0,
        .data = NULL,
    };
    return temp;
}

static SELF NAME(SELF, new_with_capacity)(size_t capacity) {
    DEBUG_ASSERT(capacity > 0);
    T* data = (T*)malloc(capacity * sizeof(T));
    if (LIKELY(data)) {
        return (SELF){
            .size = 0,
            .capacity = capacity,
            .data = data,
        };
    } else {
        PANIC;
    }
}

static SELF NAME(SELF, new_with_defaults)(size_t size, T default_value) {
    T* data = (T*)malloc(size * sizeof(T));
    for (size_t i = 0; i < size; i++) {
        data[i] = default_value;
    }
    if (LIKELY(data)) {
        return (SELF){
            .size = size,
            .capacity = size,
            .data = data,
        };
    } else {
        PANIC;
    }
}

static SELF NAME(SELF, clone)(SELF const* self) {
    DEBUG_ASSERT(self);
    T* data = (T*)malloc(self->capacity * sizeof(T));
    ASSERT(data);
    memcpy(data, self->data, self->size * sizeof(T));
    return (SELF){
        .size = self->size,
        .capacity = self->capacity,
        .data = data,
    };
}

static T const* NAME(SELF, read)(SELF const* self, size_t index) {
    DEBUG_ASSERT(self);
    return LIKELY(index < self->size) ? &self->data[index] : NULL;
}

static T* NAME(SELF, write)(SELF* self, size_t index) {
    DEBUG_ASSERT(self);
    return LIKELY(index < self->size) ? &self->data[index] : NULL;
}

static T const* NAME(SELF, read_unsafe_unchecked)(SELF const* self, size_t index) {
    DEBUG_ASSERT(self);
#ifdef NDEBUG
    T* value = NAME(SELF, read)(self, index);
    DEBUG_ASSERT(value);
    return value;
#else
    return &self->data[index];
#endif
}

static T* NAME(SELF, write_unsafe_unchecked)(SELF* self, size_t index) {
    DEBUG_ASSERT(self);
#ifdef NDEBUG
    T* value = NAME(SELF, write)(self, index);
    DEBUG_ASSERT(value);
    return value;
#else
    return &self->data[index];
#endif
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
            new_data = (T*)malloc(new_capacity * sizeof(T));
            ASSERT(new_data);
        } else {
            // JUSTIFY: Growth factor of 2
            //           - Simple arithmetic (for debugging)
            //           - Same as used by GCC's std::vector implementation
            new_capacity = self->capacity * 2;
            new_data = (T*)realloc(self->data, new_capacity * sizeof(T));
        }
        if (new_data) {
            self->capacity = new_capacity;
            self->data = new_data;
        } else {
            PANIC;
        }
    }
    T* entry = &self->data[self->size];
    *entry = value;
    self->size++;
    return entry;
}

#define POPPED_ENTRY NAME(SELF, popped_entry)

typedef struct {
    union {
        T value;
    };
    bool present;
} POPPED_ENTRY;

static POPPED_ENTRY NAME(SELF, pop)(SELF* self) {
    DEBUG_ASSERT(self);
    if (LIKELY(self->size > 0)) {
        self->size--;
        return (POPPED_ENTRY){.value = self->data[self->size], .present = true};
    } else {
        return (POPPED_ENTRY){.present = false};
    }
}

#undef POPPED_ENTRY

static size_t NAME(SELF, size)(SELF const* self) {
    DEBUG_ASSERT(self);
    return self->size;
}

static void NAME(SELF, delete)(SELF* self) {
    DEBUG_ASSERT(self);
    free(self->data);
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
    } else {
        return NULL;
    }
}

static size_t NAME(ITER, position)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos;
}

static bool NAME(ITER, empty)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos < iter->vec->size;
}

static ITER NAME(SELF, get_iter)(SELF* self) {
    DEBUG_ASSERT(self);
    return (ITER){
        .vec = self,
        .pos = 0,
    };
}

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
    } else {
        return NULL;
    }
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

#undef ITER
#undef ITER_CONST
#undef SELF
#undef T
