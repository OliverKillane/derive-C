/// @brief A vector storing the first N elements in-place, and optionally spilling additional
/// elements to a heap vector.

#include <stdint.h>

#include <derive-c/core.h>
#include <derive-c/panic.h>
#include <derive-c/self.h>

#ifndef T
#error "The contained type must be defined for a vector template"
typedef struct {
    int x;
} derive_c_parameter_t;
#define T derive_c_parameter_t // Allows independent debugging
static void derive_c_parameter_t_delete(derive_c_parameter_t*) {}
#define T_DELETE derive_c_parameter_t_delete
#endif

#ifndef T_DELETE
#define T_DELETE(value)
#endif

#ifndef INPLACE_CAPACITY
#error "The number of elements to store in-place must be defined"
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

static SELF NAME(SELF, new)() { return (SELF){.size = 0}; }

static SELF NAME(SELF, shallow_clone)(SELF const* self) {
    SELF new_self = NAME(SELF, new)();
    new_self.size = self->size;
    // TODO(oliverkillane): premature optimization, only copy the data needed.
    // If this is already a very small vector, may be better to just
    // `return *self` and copy the entire buffer
    for (INPLACE_TYPE i = 0; i < self->size; i++) {
        new_self.data[i] = self->data[i];
    }
    return new_self;
}

static T const* NAME(SELF, try_read)(SELF const* self, INPLACE_TYPE index) {
    DEBUG_ASSERT(self);
    if (LIKELY(index < self->size)) {
        return &self->data[index];
    } else {
        return NULL;
    }
}

static T const* NAME(SELF, read)(SELF const* self, INPLACE_TYPE index) {
    T const* value = NAME(SELF, try_read)(self, index);
    ASSERT(value);
    return value;
}

static T* NAME(SELF, try_write)(SELF* self, INPLACE_TYPE index) {
    DEBUG_ASSERT(self);
    if (LIKELY(index < self->size)) {
        return &self->data[index];
    } else {
        return NULL;
    }
}

static T* NAME(SELF, write)(SELF* self, INPLACE_TYPE index) {
    T* value = NAME(SELF, try_write)(self, index);
    ASSERT(value);
    return value;
}

static T* NAME(SELF, try_push)(SELF* self, T value) {
    DEBUG_ASSERT(self);
    if (self->size < INPLACE_CAPACITY) {
        T* slot = &self->data[self->size];
        *slot = value;
        self->size++;
        return slot;
    } else {
        return NULL;
    }
}

static T* NAME(SELF, push)(SELF* self, T value) {
    T* slot = NAME(SELF, try_push)(self, value);
    ASSERT(slot);
    return slot;
}

static bool NAME(SELF, try_pop)(SELF* self, T* destination) {
    DEBUG_ASSERT(self);
    if (LIKELY(self->size > 0)) {
        self->size--;
        *destination = self->data[self->size];
        return true;
    } else {
        return false;
    }
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
    for (INPLACE_TYPE i = 0; i < self->size; i++) {
        T_DELETE(&self->data[i]);
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

static T const* NAME(ITER_CONST, next)(ITER_CONST * iter) {
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

#undef ITER_CONST
#undef INPLACE_TYPE

#undef SELF
#undef T
#undef T_DELETE
#undef INPLACE_CAPACITY
