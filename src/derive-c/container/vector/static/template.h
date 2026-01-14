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
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("No ITEM")
    #endif
typedef struct {
    int x;
} item_t;
    #define ITEM item_t
    #define ITEM_DELETE item_delete
static void ITEM_DELETE(item_t* /* self */) {}
    #define ITEM_CLONE item_clone
static item_t ITEM_CLONE(item_t const* self) { return *self; }
    #define ITEM_DEBUG item_debug
static void ITEM_DEBUG(ITEM const* /* self */, dc_debug_fmt /* fmt */, FILE* /* stream */) {}
#endif

#if !defined ITEM_DELETE
    #define ITEM_DELETE DC_NO_DELETE
#endif

#if !defined ITEM_CLONE
    #define ITEM_CLONE DC_COPY_CLONE
#endif

#if !defined ITEM_DEBUG
    #define ITEM_DEBUG DC_DEFAULT_DEBUG
#endif

#if !defined CAPACITY
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("The CAPACITY must be defined")
    #endif
    #define CAPACITY 8
#endif
#if CAPACITY == 0
TEMPLATE_ERROR("CAPACITY must be greater than 0")
#elif CAPACITY <= 255
    #define INDEX_TYPE uint8_t
#elif CAPACITY <= 65535
    #define INDEX_TYPE uint16_t
#else
TEMPLATE_ERROR("CAPACITY must be less than or equal to 65535")
    #define INDEX_TYPE size_t
#endif

typedef INDEX_TYPE NS(SELF, index_t);
typedef ITEM NS(SELF, item_t);

typedef struct {
    INDEX_TYPE size;
    ITEM data[CAPACITY];
    dc_gdb_marker derive_c_staticvec;
    mutation_tracker iterator_invalidation_tracker;
} SELF;

#define INVARIANT_CHECK(self)                                                                      \
    DC_ASSUME(self);                                                                               \
    DC_ASSUME((self->size) <= CAPACITY);

static SELF NS(SELF, new)() {
    SELF self = {
        .size = 0,
        .derive_c_staticvec = dc_gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
    return self;
}

DC_STATIC_CONSTANT size_t NS(SELF, max_size) = CAPACITY;

DC_PUBLIC static SELF NS(SELF, clone)(SELF const* self) {
    SELF new_self = NS(SELF, new)();
    new_self.size = self->size;

    for (INDEX_TYPE i = 0; i < self->size; i++) {
        new_self.data[i] = ITEM_CLONE(&self->data[i]);
    }

    return new_self;
}

DC_PUBLIC static ITEM const* NS(SELF, try_read)(SELF const* self, INDEX_TYPE index) {
    INVARIANT_CHECK(self);
    if (DC_LIKELY(index < self->size)) {
        return &self->data[index];
    }
    return NULL;
}

DC_PUBLIC static ITEM const* NS(SELF, read)(SELF const* self, INDEX_TYPE index) {
    ITEM const* value = NS(SELF, try_read)(self, index);
    DC_ASSERT(value);
    return value;
}

DC_PUBLIC static ITEM* NS(SELF, try_write)(SELF* self, INDEX_TYPE index) {
    INVARIANT_CHECK(self);
    if (DC_LIKELY(index < self->size)) {
        return &self->data[index];
    }
    return NULL;
}

DC_PUBLIC static ITEM* NS(SELF, write)(SELF* self, INDEX_TYPE index) {
    ITEM* value = NS(SELF, try_write)(self, index);
    DC_ASSERT(value);
    return value;
}

DC_PUBLIC static ITEM* NS(SELF, try_push)(SELF* self, ITEM item) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    if (self->size < CAPACITY) {
        ITEM* slot = &self->data[self->size];
        *slot = item;
        self->size++;
        return slot;
    }
    return NULL;
}

DC_PUBLIC static ITEM* NS(SELF, try_insert_at)(SELF* self, INDEX_TYPE at, ITEM const* items,
                                               INDEX_TYPE count) {
    INVARIANT_CHECK(self);
    DC_ASSUME(items);
    DC_ASSERT(at <= self->size);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    if (self->size + count > CAPACITY) {
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

DC_PUBLIC static void NS(SELF, remove_at)(SELF* self, INDEX_TYPE at, INDEX_TYPE count) {
    INVARIANT_CHECK(self);
    DC_ASSERT(at + count <= self->size);

    if (count == 0) {
        return;
    }

    for (INDEX_TYPE i = at; i < at + count; i++) {
        ITEM_DELETE(&self->data[i]);
    }

    memmove(&self->data[at], &self->data[at + count], (self->size - (at + count)) * sizeof(ITEM));
    self->size -= count;
}

DC_PUBLIC static ITEM* NS(SELF, push)(SELF* self, ITEM item) {
    ITEM* slot = NS(SELF, try_push)(self, item);
    DC_ASSERT(slot);
    return slot;
}

DC_PUBLIC static bool NS(SELF, try_pop)(SELF* self, ITEM* destination) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    if (DC_LIKELY(self->size > 0)) {
        self->size--;
        *destination = self->data[self->size];
        return true;
    }
    return false;
}

DC_PUBLIC static ITEM NS(SELF, pop)(SELF* self) {
    ITEM entry;
    DC_ASSERT(NS(SELF, try_pop)(self, &entry));
    return entry;
}

DC_PUBLIC static INDEX_TYPE NS(SELF, size)(SELF const* self) {
    INVARIANT_CHECK(self);
    return self->size;
}

DC_PUBLIC static void NS(SELF, delete)(SELF* self) {
    INVARIANT_CHECK(self);
    for (INDEX_TYPE i = 0; i < self->size; i++) {
        ITEM_DELETE(&self->data[i]);
    }
}

#define ITER NS(SELF, iter)
typedef ITEM* NS(ITER, item);

DC_PUBLIC static bool NS(ITER, empty_item)(ITEM* const* item) { return *item == NULL; }

typedef struct {
    SELF* vec;
    INDEX_TYPE pos;
    mutation_version version;
} ITER;

DC_PUBLIC static ITEM* NS(ITER, next)(ITER* iter) {
    DC_ASSUME(iter);
    mutation_version_check(&iter->version);
    if (iter->pos < iter->vec->size) {
        ITEM* item = &iter->vec->data[iter->pos];
        iter->pos++;
        return item;
    }
    return NULL;
}

DC_PUBLIC static INDEX_TYPE NS(ITER, position)(ITER const* iter) {
    DC_ASSUME(iter);
    mutation_version_check(&iter->version);
    return iter->pos;
}

DC_PUBLIC static bool NS(ITER, empty)(ITER const* iter) {
    DC_ASSUME(iter);
    mutation_version_check(&iter->version);
    return iter->pos >= iter->vec->size;
}

DC_PUBLIC static ITER NS(SELF, get_iter)(SELF* self) {
    DC_ASSUME(self);
    return (ITER){.vec = self,
                  .pos = 0,
                  .version = mutation_tracker_get(&self->iterator_invalidation_tracker)};
}

#undef ITER

#define ITER_CONST NS(SELF, iter_const)
typedef ITEM const* NS(ITER_CONST, item);

DC_PUBLIC static bool NS(ITER_CONST, empty_item)(ITEM const* const* item) { return *item == NULL; }

typedef struct {
    SELF const* vec;
    INDEX_TYPE pos;
    mutation_version version;
} ITER_CONST;

DC_PUBLIC static ITEM const* NS(ITER_CONST, next)(ITER_CONST* iter) {
    DC_ASSUME(iter);
    mutation_version_check(&iter->version);
    if (iter->pos < iter->vec->size) {
        ITEM const* item = &iter->vec->data[iter->pos];
        iter->pos++;
        return item;
    }
    return NULL;
}

DC_PUBLIC static INDEX_TYPE NS(ITER_CONST, position)(ITER_CONST const* iter) {
    DC_ASSUME(iter);
    mutation_version_check(&iter->version);
    return iter->pos;
}

DC_PUBLIC static bool NS(ITER_CONST, empty)(ITER_CONST const* iter) {
    DC_ASSUME(iter);
    mutation_version_check(&iter->version);
    return iter->pos >= iter->vec->size;
}

DC_PUBLIC static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    DC_ASSUME(self);
    return (ITER_CONST){
        .vec = self,
        .pos = 0,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

DC_PUBLIC static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, DC_EXPAND_STRING(SELF) "@%p {\n", (void*)self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "capacity: %lu,\n", (size_t)CAPACITY);
    dc_debug_fmt_print(fmt, stream, "size: %lu,\n", (size_t)self->size);

    dc_debug_fmt_print(fmt, stream, "items: @%p [\n", (void*)self->data);
    fmt = dc_debug_fmt_scope_begin(fmt);
    ITER_CONST iter = NS(SELF, get_iter_const)(self);
    ITEM const* item;
    while ((item = NS(ITER_CONST, next)(&iter))) {
        dc_debug_fmt_print_indents(fmt, stream);
        ITEM_DEBUG(item, fmt, stream);
        fprintf(stream, ",\n");
    }
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "],\n");
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#undef ITER_CONST
#undef INVARIANT_CHECK
#undef INDEX_TYPE
#undef CAPACITY
#undef ITEM_DEBUG
#undef ITEM_CLONE
#undef ITEM_DELETE
#undef ITEM

DC_TRAIT_VECTOR(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/includes/undef.h>
