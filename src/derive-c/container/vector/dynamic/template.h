/// @brief A simple vector

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <derive-c/container/vector/trait.h>
#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>
#include <derive-c/core/placeholder.h>

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined ITEM
    #if !defined PLACEHOLDERS
        #error "The contained type must be defined for a vector template"
    #endif
typedef struct {
    int x;
} item_t;
    #define ITEM item_t
static void item_delete(item_t* UNUSED(t)) {}
    #define ITEM_DELETE item_delete
static item_t item_clone(item_t const* i) { return *i; }
    #define ITEM_CLONE item_clone
#endif

#if !defined ITEM_DELETE
    #define ITEM_DELETE(value)
#endif

#if !defined ITEM_CLONE
    #define ITEM_CLONE(value) (*(value))
#endif

typedef size_t NS(SELF, index_t);
typedef ITEM NS(SELF, item_t);

typedef struct {
    size_t size;
    size_t capacity;
    ITEM* data;
    ALLOC* alloc;
    gdb_marker derive_c_vector;
} SELF;

static SELF NS(SELF, new)(ALLOC* alloc) {
    SELF temp = (SELF){
        .size = 0,
        .capacity = 0,
        .data = NULL,
        .alloc = alloc,
    };
    return temp;
}

static SELF NS(SELF, new_with_capacity)(size_t capacity, ALLOC* alloc) {
    if (capacity == 0) {
        return NS(SELF, new)(alloc);
    }

    ITEM* data = (ITEM*)NS(ALLOC, malloc)(alloc, capacity * sizeof(ITEM));
    ASSERT(LIKELY(data));
    return (SELF){
        .size = 0,
        .capacity = capacity,
        .data = data,
        .alloc = alloc,
    };
}

static SELF NS(SELF, new_with_defaults)(size_t size, ITEM default_item, ALLOC* alloc) {
    ITEM* data = (ITEM*)NS(ALLOC, malloc)(alloc, size * sizeof(ITEM));
    if (size > 0) {
        // JUSTIFY: We only need to copy size-1 entries - can move the first as default.
        data[0] = default_item;
        for (size_t i = 1; i < size; i++) {
            data[i] = ITEM_CLONE(&default_item);
        }
    }
    ASSERT(LIKELY(data));
    return (SELF){
        .size = size,
        .capacity = size,
        .data = data,
        .alloc = alloc,
    };
}

static void NS(SELF, reserve)(SELF* self, size_t new_capacity) {
    DEBUG_ASSERT(self);
    if (new_capacity > self->capacity) {
        ITEM* new_data =
            (ITEM*)NS(ALLOC, realloc)(self->alloc, self->data, new_capacity * sizeof(ITEM));
        ASSERT(new_data);
        self->data = new_data;
        self->capacity = new_capacity;
    }
}

static SELF NS(SELF, clone)(SELF const* self) {
    DEBUG_ASSERT(self);
    ITEM* data = (ITEM*)NS(ALLOC, malloc)(self->alloc, self->capacity * sizeof(ITEM));
    ASSERT(data);
    for (size_t index = 0; index < self->size; index++) {
        data[index] = ITEM_CLONE(&self->data[index]);
    }
    return (SELF){
        .size = self->size,
        .capacity = self->capacity,
        .data = data,
        .alloc = self->alloc,
    };
}

static ITEM const* NS(SELF, try_read)(SELF const* self, size_t index) {
    DEBUG_ASSERT(self);
    if (LIKELY(index < self->size)) {
        return &self->data[index];
    }
    return NULL;
}

static ITEM const* NS(SELF, read)(SELF const* self, size_t index) {
    ITEM const* item = NS(SELF, try_read)(self, index);
    ASSERT(item);
    return item;
}

static ITEM* NS(SELF, try_write)(SELF* self, size_t index) {
    DEBUG_ASSERT(self);
    if (LIKELY(index < self->size)) {
        return &self->data[index];
    }
    return NULL;
}

static ITEM* NS(SELF, write)(SELF* self, size_t index) {
    ITEM* item = NS(SELF, try_write)(self, index);
    ASSERT(item);
    return item;
}

static void NS(SELF, insert_at)(SELF* self, size_t at, ITEM const* items, size_t count) {
    DEBUG_ASSERT(self);
    DEBUG_ASSERT(items);
    ASSERT(at <= self->size);

    if (count == 0) {
        return;
    }

    NS(SELF, reserve)(self, self->size + count);

    memmove(&self->data[at + count], &self->data[at], (self->size - at) * sizeof(ITEM));
    memcpy(&self->data[at], items, count * sizeof(ITEM));
    self->size += count;
}

static void NS(SELF, remove_at)(SELF* self, size_t at, size_t count) {
    DEBUG_ASSERT(self);
    ASSERT(at + count <= self->size);

    if (count == 0) {
        return;
    }

    for (size_t i = at; i < at + count; i++) {
        ITEM_DELETE(&self->data[i]);
    }

    memmove(&self->data[at], &self->data[at + count], (self->size - (at + count)) * sizeof(ITEM));
    self->size -= count;
}

static ITEM* NS(SELF, push)(SELF* self, ITEM item) {
    DEBUG_ASSERT(self);
    if (self->size == self->capacity) {
        ITEM* new_data;
        size_t new_capacity;
        if (self->data == NULL) {
            DEBUG_ASSERT(self->capacity == 0);
            // JUSTIFY: Allocating capacity of 4
            //           - Avoid repeat reallocations on growing a vector from
            //             size sero (from new)
            //           Otherwise an arbitrary choice (given we do not know the size of T)
            new_capacity = 8;
            new_data = (ITEM*)NS(ALLOC, malloc)(self->alloc, new_capacity * sizeof(ITEM));
        } else {
            // JUSTIFY: Growth factor of 2
            //           - Simple arithmetic (for debugging)
            //           - Same as used by GCC's std::vector implementation
            new_capacity = self->capacity * 2;
            new_data =
                (ITEM*)NS(ALLOC, realloc)(self->alloc, self->data, new_capacity * sizeof(ITEM));
        }
        ASSERT(new_data);
        self->capacity = new_capacity;
        self->data = new_data;
    }
    ITEM* entry = &self->data[self->size];
    *entry = item;
    self->size++;
    return entry;
}

static bool NS(SELF, try_pop)(SELF* self, ITEM* destination) {
    DEBUG_ASSERT(self);
    if (LIKELY(self->size > 0)) {
        self->size--;
        *destination = self->data[self->size];
        return true;
    }
    return false;
}

static ITEM* NS(SELF, data)(SELF* self) {
    DEBUG_ASSERT(self);
    return self->data;
}

static ITEM NS(SELF, pop)(SELF* self) {
    ITEM entry;
    ASSERT(NS(SELF, try_pop)(self, &entry));
    return entry;
}

static ITEM NS(SELF, pop_front)(SELF* self) {
    DEBUG_ASSERT(self);
    ASSERT(self->size > 0);
    ITEM entry = self->data[0];
    memmove(&self->data[0], &self->data[1], (self->size - 1) * sizeof(ITEM));
    self->size--;
    return entry;
}

static size_t NS(SELF, size)(SELF const* self) {
    DEBUG_ASSERT(self);
    return self->size;
}

static void NS(SELF, delete)(SELF* self) {
    DEBUG_ASSERT(self);
    if (self->data) {
        for (size_t i = 0; i < self->size; i++) {
            ITEM_DELETE(&self->data[i]);
        }
        NS(ALLOC, free)(self->alloc, self->data);
    }
}

#define ITER NS(SELF, iter)
typedef ITEM* NS(ITER, item);

static bool NS(ITER, empty_item)(ITEM* const* item) { return *item == NULL; }

typedef struct {
    SELF* vec;
    size_t pos;
} ITER;

static ITEM* NS(ITER, next)(ITER* iter) {
    DEBUG_ASSERT(iter);
    if (iter->pos < iter->vec->size) {
        ITEM* item = &iter->vec->data[iter->pos];
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
typedef ITEM const* NS(ITER_CONST, item);

static bool NS(ITER_CONST, empty_item)(ITEM const* const* item) { return *item == NULL; }

typedef struct {
    SELF const* vec;
    size_t pos;
} ITER_CONST;

static ITEM const* NS(ITER_CONST, next)(ITER_CONST* iter) {
    DEBUG_ASSERT(iter);
    if (iter->pos < iter->vec->size) {
        ITEM const* item = &iter->vec->data[iter->pos];
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

#undef ITEM
#undef ITEM_DELETE
#undef ITEM_CLONE

#include <derive-c/core/alloc/undef.h>
TRAIT_VECTOR(SELF);

#include <derive-c/core/self/undef.h>
