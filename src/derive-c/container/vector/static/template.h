/// @brief A vector storing the first N elements in-place, and optionally spilling additional
/// elements to a heap vector.

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

// JUSTIFY: No memory tracking
//  - The vector is in-place, inside the vector object itself
//  - poisoning this makes memcopying the struct throw in asan
// On balance, the likelihood of hitting frustrating issues (e.g. from cpp copying objects, C
// structs trivially copyable) is high, and the benefit of catching improper usage of vector
// elements is not high enough to overcome.

#include <derive-c/core/self/def.h>

#if !defined ITEM
    #if !defined PLACEHOLDERS
TEMPLATE_ERROR("No ITEM")
    #endif
typedef struct {
    int x;
} item_t;
    #define ITEM item_t
    #define ITEM_DELETE item_delete
static void ITEM_DELETE(item_t* self);
    #define ITEM_CLONE item_clone
static item_t ITEM_CLONE(item_t const* self);
    #define ITEM_DEBUG item_debug
static void ITEM_DEBUG(ITEM const* self, debug_fmt fmt, FILE* stream);
#endif

#if !defined ITEM_DELETE
    #define ITEM_DELETE NO_DELETE
#endif

#if !defined ITEM_CLONE
    #define ITEM_CLONE COPY_CLONE
#endif

#if !defined ITEM_DEBUG
    #define ITEM_DEBUG DEFAULT_DEBUG
#endif

#if !defined INPLACE_CAPACITY
    #if !defined PLACEHOLDERS
TEMPLATE_ERROR("The INPLACE_CAPACITY must be defined")
    #endif
    #define INPLACE_CAPACITY 8
#endif
#if INPLACE_CAPACITY == 0
TEMPLATE_ERROR("INPLACE_CAPACITY must be greater than 0")
#elif INPLACE_CAPACITY <= 255
    #define INDEX_TYPE uint8_t
#elif INPLACE_CAPACITY <= 65535
    #define INDEX_TYPE uint16_t
#else
TEMPLATE_ERROR("INPLACE_CAPACITY must be less than or equal to 65535")
    #define INDEX_TYPE size_t
#endif

typedef INDEX_TYPE NS(SELF, index_t);
typedef ITEM NS(SELF, item_t);

typedef struct {
    INDEX_TYPE size;
    ITEM data[INPLACE_CAPACITY];
    gdb_marker derive_c_staticvec;
    mutation_tracker iterator_invalidation_tracker;
} SELF;

#define INVARIANT_CHECK(self)                                                                      \
    ASSUME(self);                                                                                  \
    ASSUME((self->size) <= INPLACE_CAPACITY);

static SELF NS(SELF, new)() {
    SELF self = {
        .size = 0,
        .derive_c_staticvec = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
    return self;
}

static size_t NS(SELF, max_size)() { return INPLACE_CAPACITY; }

static SELF NS(SELF, clone)(SELF const* self) {
    SELF new_self = NS(SELF, new)();
    new_self.size = self->size;

    for (INDEX_TYPE i = 0; i < self->size; i++) {
        new_self.data[i] = ITEM_CLONE(&self->data[i]);
    }

    return new_self;
}

static ITEM const* NS(SELF, try_read)(SELF const* self, INDEX_TYPE index) {
    INVARIANT_CHECK(self);
    if (LIKELY(index < self->size)) {
        return &self->data[index];
    }
    return NULL;
}

static ITEM const* NS(SELF, read)(SELF const* self, INDEX_TYPE index) {
    ITEM const* value = NS(SELF, try_read)(self, index);
    ASSERT(value);
    return value;
}

static ITEM* NS(SELF, try_write)(SELF* self, INDEX_TYPE index) {
    INVARIANT_CHECK(self);
    if (LIKELY(index < self->size)) {
        return &self->data[index];
    }
    return NULL;
}

static ITEM* NS(SELF, write)(SELF* self, INDEX_TYPE index) {
    ITEM* value = NS(SELF, try_write)(self, index);
    ASSERT(value);
    return value;
}

static ITEM* NS(SELF, try_push)(SELF* self, ITEM item) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    if (self->size < INPLACE_CAPACITY) {
        ITEM* slot = &self->data[self->size];
        *slot = item;
        self->size++;
        return slot;
    }
    return NULL;
}

static ITEM* NS(SELF, try_insert_at)(SELF* self, INDEX_TYPE at, ITEM const* items,
                                     INDEX_TYPE count) {
    INVARIANT_CHECK(self);
    ASSUME(items);
    ASSERT(at <= self->size);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    if (self->size + count > INPLACE_CAPACITY) {
        return NULL;
    }

    if (count == 0) {
        return NULL;
    }

    memmove(&self->data[at + count], &self->data[at], (self->size - at) * sizeof(ITEM));
    memcpy(&self->data[at], items, count * sizeof(ITEM));
    self->size += count;
    return &self->data[at];
}

static void NS(SELF, remove_at)(SELF* self, INDEX_TYPE at, INDEX_TYPE count) {
    INVARIANT_CHECK(self);
    ASSERT(at + count <= self->size);

    if (count == 0) {
        return;
    }

    for (INDEX_TYPE i = at; i < at + count; i++) {
        ITEM_DELETE(&self->data[i]);
    }

    memmove(&self->data[at], &self->data[at + count], (self->size - (at + count)) * sizeof(ITEM));
    self->size -= count;
}

static ITEM* NS(SELF, push)(SELF* self, ITEM item) {
    ITEM* slot = NS(SELF, try_push)(self, item);
    ASSERT(slot);
    return slot;
}

static bool NS(SELF, try_pop)(SELF* self, ITEM* destination) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    if (LIKELY(self->size > 0)) {
        self->size--;
        *destination = self->data[self->size];
        return true;
    }
    return false;
}

static ITEM NS(SELF, pop)(SELF* self) {
    ITEM entry;
    ASSERT(NS(SELF, try_pop)(self, &entry));
    return entry;
}

static INDEX_TYPE NS(SELF, size)(SELF const* self) {
    INVARIANT_CHECK(self);
    return self->size;
}

static void NS(SELF, delete)(SELF* self) {
    INVARIANT_CHECK(self);
    for (INDEX_TYPE i = 0; i < self->size; i++) {
        ITEM_DELETE(&self->data[i]);
    }
}

#define ITER NS(SELF, iter)
typedef ITEM* NS(ITER, item);

static bool NS(ITER, empty_item)(ITEM* const* item) { return *item == NULL; }

typedef struct {
    SELF* vec;
    INDEX_TYPE pos;
    mutation_version version;
} ITER;

static ITEM* NS(ITER, next)(ITER* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);
    if (iter->pos < iter->vec->size) {
        ITEM* item = &iter->vec->data[iter->pos];
        iter->pos++;
        return item;
    }
    return NULL;
}

static INDEX_TYPE NS(ITER, position)(ITER const* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);
    return iter->pos;
}

static bool NS(ITER, empty)(ITER const* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);
    return iter->pos >= iter->vec->size;
}

static ITER NS(SELF, get_iter)(SELF* self) {
    ASSUME(self);
    return (ITER){.vec = self,
                  .pos = 0,
                  .version = mutation_tracker_get(&self->iterator_invalidation_tracker)};
}

#undef ITER

#define ITER_CONST NS(SELF, iter_const)
typedef ITEM const* NS(ITER_CONST, item);

static bool NS(ITER_CONST, empty_item)(ITEM const* const* item) { return *item == NULL; }

typedef struct {
    SELF const* vec;
    INDEX_TYPE pos;
    mutation_version version;
} ITER_CONST;

static ITEM const* NS(ITER_CONST, next)(ITER_CONST* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);
    if (iter->pos < iter->vec->size) {
        ITEM const* item = &iter->vec->data[iter->pos];
        iter->pos++;
        return item;
    }
    return NULL;
}

static INDEX_TYPE NS(ITER_CONST, position)(ITER_CONST const* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);
    return iter->pos;
}

static bool NS(ITER_CONST, empty)(ITER_CONST const* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);
    return iter->pos >= iter->vec->size;
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    ASSUME(self);
    return (ITER_CONST){
        .vec = self,
        .pos = 0,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

static void NS(SELF, debug)(SELF const* self, debug_fmt fmt, FILE* stream) {
    fprintf(stream, EXPAND_STRING(SELF) "@%p {\n", self);
    fmt = debug_fmt_scope_begin(fmt);
    debug_fmt_print(fmt, stream, "capacity: %lu,\n", (size_t)INPLACE_CAPACITY);
    debug_fmt_print(fmt, stream, "size: %lu,\n", (size_t)self->size);

    debug_fmt_print(fmt, stream, "items: @%p [\n", self->data);
    fmt = debug_fmt_scope_begin(fmt);
    ITER_CONST iter = NS(SELF, get_iter_const)(self);
    ITEM const* item;
    while ((item = NS(ITER_CONST, next)(&iter))) {
        debug_fmt_print_indents(fmt, stream);
        ITEM_DEBUG(item, fmt, stream);
        fprintf(stream, ",\n");
    }
    fmt = debug_fmt_scope_end(fmt);
    debug_fmt_print(fmt, stream, "],\n");
    fmt = debug_fmt_scope_end(fmt);
    debug_fmt_print(fmt, stream, "}\n");
}

#undef ITER_CONST
#undef INVARIANT_CHECK
#undef INDEX_TYPE
#undef INPLACE_CAPACITY
#undef ITEM_DEBUG
#undef ITEM_CLONE
#undef ITEM_DELETE
#undef ITEM

DC_TRAIT_VECTOR(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/includes/undef.h>
