/// @brief A simple vector

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/alloc/def.h>
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

typedef size_t NS(SELF, index_t);
typedef ITEM NS(SELF, item_t);

typedef struct {
    size_t size;
    size_t capacity;
    ITEM* data;
    ALLOC* alloc;
    gdb_marker derive_c_vector_marker;
    mutation_tracker iterator_invalidation_tracker;
} SELF;

#define INVARIANT_CHECK(self)                                                                      \
    ASSUME(self);                                                                                  \
    ASSUME((self)->size <= (self)->capacity);                                                      \
    ASSUME((self->alloc));                                                                         \
    ASSUME(WHEN(!((self)->data), (self)->capacity == 0 && (self)->size == 0));

static size_t NS(SELF, max_size)() { return SIZE_MAX; }

static SELF NS(SELF, new)(ALLOC* alloc) {
    SELF self = (SELF){
        .size = 0,
        .capacity = 0,
        .data = NULL,
        .alloc = alloc,
        .derive_c_vector_marker = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
    return self;
}

static SELF NS(SELF, new_with_capacity)(size_t capacity, ALLOC* alloc) {
    if (capacity == 0) {
        return NS(SELF, new)(alloc);
    }

    ITEM* data = (ITEM*)NS(ALLOC, malloc)(alloc, capacity * sizeof(ITEM));
    ASSERT(LIKELY(data));
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE, data,
                       capacity * sizeof(ITEM));
    return (SELF){
        .size = 0,
        .capacity = capacity,
        .data = data,
        .alloc = alloc,
        .derive_c_vector_marker = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
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
        .derive_c_vector_marker = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static void NS(SELF, reserve)(SELF* self, size_t new_capacity) {
    INVARIANT_CHECK(self);
    if (new_capacity > self->capacity) {
        const size_t capacity_increase = new_capacity - self->capacity;
        const size_t uninit_elements = self->capacity - self->size;

        // Set to writeable for allocator
        memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_WRITE,
                           &self->data[self->size], uninit_elements * sizeof(ITEM));

        ITEM* new_data =
            (ITEM*)NS(ALLOC, realloc)(self->alloc, self->data, new_capacity * sizeof(ITEM));
        ASSERT(new_data);
        self->data = new_data;
        memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE,
                           &self->data[self->size],
                           (uninit_elements + capacity_increase) * sizeof(ITEM));
        self->capacity = new_capacity;
    }
}

static SELF NS(SELF, clone)(SELF const* self) {
    INVARIANT_CHECK(self);
    ITEM* data = (ITEM*)NS(ALLOC, malloc)(self->alloc, self->capacity * sizeof(ITEM));
    ASSERT(data);
    for (size_t index = 0; index < self->size; index++) {
        data[index] = ITEM_CLONE(&self->data[index]);
    }
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE, &data[self->size],
                       (self->capacity - self->size) * sizeof(ITEM));
    return (SELF){
        .size = self->size,
        .capacity = self->capacity,
        .data = data,
        .alloc = self->alloc,
        .derive_c_vector_marker = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static ITEM const* NS(SELF, try_read)(SELF const* self, size_t index) {
    INVARIANT_CHECK(self);
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
    INVARIANT_CHECK(self);
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

static ITEM* NS(SELF, try_insert_at)(SELF* self, size_t at, ITEM const* items, size_t count) {
    INVARIANT_CHECK(self);
    ASSUME(items);
    ASSERT(at <= self->size);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    if (count == 0) {
        return NULL;
    }

    NS(SELF, reserve)(self, self->size + count);
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_WRITE,
                       &self->data[self->size], count * sizeof(ITEM));
    memmove(&self->data[at + count], &self->data[at], (self->size - at) * sizeof(ITEM));
    memcpy(&self->data[at], items, count * sizeof(ITEM));
    self->size += count;
    return &self->data[at];
}

static void NS(SELF, remove_at)(SELF* self, size_t at, size_t count) {
    INVARIANT_CHECK(self);
    ASSERT(at + count <= self->size);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    if (count == 0) {
        return;
    }

    for (size_t i = at; i < at + count; i++) {
        ITEM_DELETE(&self->data[i]);
    }

    memmove(&self->data[at], &self->data[at + count], (self->size - (at + count)) * sizeof(ITEM));
    self->size -= count;
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE,
                       &self->data[self->size], count * sizeof(ITEM));
}

static ITEM* NS(SELF, push)(SELF* self, ITEM item) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    if (self->size == self->capacity) {
        size_t new_capacity;
        if (self->data == NULL) {
            ASSUME(self->capacity == 0);
            // JUSTIFY: Allocating capacity of 4
            //           - Avoid repeat reallocations on growing a vector from
            //             size sero (from new)
            //           Otherwise an arbitrary choice (given we do not know the size of T)
            new_capacity = 8;
        } else {
            // JUSTIFY: Growth factor of 2
            //           - Simple arithmetic (for debugging)
            //           - Same as used by GCC's std::vector implementation
            new_capacity = self->capacity * 2;
        }
        NS(SELF, reserve)(self, new_capacity);
    }

    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_WRITE,
                       &self->data[self->size], sizeof(ITEM));

    ITEM* entry = &self->data[self->size];
    *entry = item;
    self->size++;
    return entry;
}

static bool NS(SELF, try_pop)(SELF* self, ITEM* destination) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    if (LIKELY(self->size > 0)) {
        self->size--;
        *destination = self->data[self->size];
        memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE,
                           &self->data[self->size + 1], sizeof(ITEM));
        return true;
    }
    return false;
}

static ITEM* NS(SELF, data)(SELF* self) {
    INVARIANT_CHECK(self);
    return self->data;
}

static ITEM NS(SELF, pop)(SELF* self) {
    ITEM entry;
    ASSERT(NS(SELF, try_pop)(self, &entry));
    return entry;
}

static ITEM NS(SELF, pop_front)(SELF* self) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    ASSERT(self->size > 0);
    ITEM entry = self->data[0];
    memmove(&self->data[0], &self->data[1], (self->size - 1) * sizeof(ITEM));
    self->size--;
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE,
                       &self->data[self->size], sizeof(ITEM));
    return entry;
}

static size_t NS(SELF, size)(SELF const* self) {
    INVARIANT_CHECK(self);
    return self->size;
}

static void NS(SELF, delete)(SELF* self) {
    INVARIANT_CHECK(self);
    if (self->data) {
        for (size_t i = 0; i < self->size; i++) {
            ITEM_DELETE(&self->data[i]);
            // JUSTIFY: Setting items as inaccessible
            //  - Incase a destructor of one item accesses the memory of another.
            memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE,
                               &self->data[i], sizeof(ITEM));
        }

        // JUSTIFY: Return to write level before passing to allocator
        //  - Is uninitialised, but still valid memory
        memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_WRITE, self->data,
                           self->capacity * sizeof(ITEM));
        NS(ALLOC, free)(self->alloc, self->data);
    }
}

/// Moves `to_move` items from the beginning of source, to the end of target,
/// shuffling elements in source forward appropriately.
/// - Used for `deque` rebalancing
///
/// ```
/// index:   0  1  2  3  4  5
/// source: [5, 4, 3, 2, 1, 0]
/// target: [6, 7, 8, 9]
/// ```
/// Becomes:
/// ```
/// index:   0  1  2  3  4  5
/// source: [3, 2, 1, 0]
/// target: [4, 5, 6, 7, 8, 9]
/// ```
static void NS(SELF, transfer_reverse)(SELF* source, SELF* target, size_t to_move) {
    INVARIANT_CHECK(source);
    INVARIANT_CHECK(target);

    NS(SELF, reserve)(target, target->size + to_move);

    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_WRITE,
                       &target->data[target->size], to_move * sizeof(ITEM));
    memmove(&target->data[to_move], target->data, target->size * sizeof(ITEM));

    for (size_t i = 0; i < to_move; i++) {
        target->data[to_move - 1 - i] = source->data[i];
    }

    memmove(source->data, &source->data[to_move], (source->size - to_move) * sizeof(ITEM));

    source->size -= to_move;
    target->size += to_move;
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE,
                       &source->data[source->size], to_move * sizeof(ITEM));
}

#define ITER NS(SELF, iter)
typedef ITEM* NS(ITER, item);

static bool NS(ITER, empty_item)(ITEM* const* item) { return *item == NULL; }

typedef struct {
    SELF* vec;
    size_t pos;
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

static size_t NS(ITER, position)(ITER const* iter) {
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
    return (ITER){
        .vec = self,
        .pos = 0,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}
#undef ITER

#define ITER_CONST NS(SELF, iter_const)
typedef ITEM const* NS(ITER_CONST, item);

static bool NS(ITER_CONST, empty_item)(ITEM const* const* item) { return *item == NULL; }

typedef struct {
    SELF const* vec;
    size_t pos;
    mutation_version vec_version;
} ITER_CONST;

static ITEM const* NS(ITER_CONST, next)(ITER_CONST* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->vec_version);
    if (iter->pos < iter->vec->size) {
        ITEM const* item = &iter->vec->data[iter->pos];
        iter->pos++;
        return item;
    }
    return NULL;
}

static size_t NS(ITER_CONST, position)(ITER_CONST const* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->vec_version);
    return iter->pos;
}

static bool NS(ITER_CONST, empty)(ITER_CONST const* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->vec_version);
    return iter->pos >= iter->vec->size;
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    ASSUME(self);
    return (ITER_CONST){
        .vec = self,
        .pos = 0,
        .vec_version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

static void NS(SELF, debug)(SELF const* self, debug_fmt fmt, FILE* stream) {
    fprintf(stream, EXPAND_STRING(SELF) "@%p {\n", self);
    fmt = debug_fmt_scope_begin(fmt);
    debug_fmt_print(fmt, stream, "size: %lu,\n", self->size);
    debug_fmt_print(fmt, stream, "capacity: %lu,\n", self->capacity);

    debug_fmt_print(fmt, stream, "alloc: ");
    NS(ALLOC, debug)(self->alloc, fmt, stream);
    fprintf(stream, ",\n");

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
    debug_fmt_print(fmt, stream, "}");
}

#undef ITER_CONST
#undef INVARIANT_CHECK
#undef ITEM_DEBUG
#undef ITEM_CLONE
#undef ITEM_DELETE
#undef ITEM

DC_TRAIT_VECTOR(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/includes/undef.h>