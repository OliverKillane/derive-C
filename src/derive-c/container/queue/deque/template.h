/// @brief A simple double ended queue

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

typedef ITEM NS(SELF, item_t);
typedef ALLOC NS(SELF, alloc_t);

#define ITEM_VECTORS NS(NAME, item_vectors)

#pragma push_macro("ALLOC")
#pragma push_macro("ITEM")
#pragma push_macro("ITEM_CLONE")
#pragma push_macro("ITEM_DELETE")

// ITEM is already defined
#define INTERNAL_NAME ITEM_VECTORS // [DERIVE-C] for template
#include <derive-c/container/vector/dynamic/template.h>

#pragma pop_macro("ALLOC")
#pragma pop_macro("ITEM")
#pragma pop_macro("ITEM_CLONE")
#pragma pop_macro("ITEM_DELETE")

typedef struct {
    ITEM_VECTORS front;
    ITEM_VECTORS back;
    gdb_marker derive_c_deque;
    mutation_tracker iterator_invalidation_tracker;
} SELF;

#define INVARIANT_CHECK(self) ASSUME(self);

static SELF NS(SELF, new)(ALLOC* alloc) {
    return (SELF){
        .front = NS(ITEM_VECTORS, new)(alloc),
        .back = NS(ITEM_VECTORS, new)(alloc),
        .derive_c_deque = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static SELF NS(SELF, new_with_capacity)(size_t front_and_back_capacity, ALLOC* alloc) {
    return (SELF){
        .front = NS(ITEM_VECTORS, new_with_capacity)(front_and_back_capacity, alloc),
        .back = NS(ITEM_VECTORS, new_with_capacity)(front_and_back_capacity, alloc),
        .derive_c_deque = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static SELF NS(SELF, clone)(SELF const* other) {
    INVARIANT_CHECK(other);
    return (SELF){
        .front = NS(ITEM_VECTORS, clone)(&other->front),
        .back = NS(ITEM_VECTORS, clone)(&other->back),
        .derive_c_deque = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static size_t NS(SELF, size)(SELF const* self) {
    INVARIANT_CHECK(self);
    return NS(ITEM_VECTORS, size)(&self->front) + NS(ITEM_VECTORS, size)(&self->back);
}

static bool NS(SELF, empty)(SELF const* self) {
    INVARIANT_CHECK(self);
    return NS(ITEM_VECTORS, size)(&self->front) == 0 && NS(ITEM_VECTORS, size)(&self->back) == 0;
}

static void NS(SELF, rebalance)(SELF* self) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    size_t const front_size = NS(ITEM_VECTORS, size)(&self->front);
    size_t const back_size = NS(ITEM_VECTORS, size)(&self->back);
    size_t const total_size = front_size + back_size;

    if (!deque_rebalance_policy(total_size, front_size)) {
        return;
    }

    ITEM_VECTORS* source;
    ITEM_VECTORS* target;
    size_t source_size;
    size_t target_size;

    if (front_size > back_size + 1) {
        source = &self->front;
        target = &self->back;
        source_size = front_size;
        target_size = back_size;
    } else {
        ASSUME(back_size > front_size + 1);
        source = &self->back;
        target = &self->front;
        source_size = back_size;
        target_size = front_size;
    }

    size_t const to_move = (source_size - target_size) / 2;
    NS(ITEM_VECTORS, transfer_reverse)(source, target, to_move);
}

static ITEM const* NS(SELF, try_read_from_front)(SELF const* self, size_t index) {
    INVARIANT_CHECK(self);

    if (index < NS(ITEM_VECTORS, size)(&self->front)) {
        size_t front_index = NS(ITEM_VECTORS, size)(&self->front) - 1 - index;
        return NS(ITEM_VECTORS, read)(&self->front, front_index);
    }

    size_t back_index = index - NS(ITEM_VECTORS, size)(&self->front);
    return NS(ITEM_VECTORS, try_read)(&self->back, back_index);
}

static ITEM const* NS(SELF, try_read_from_back)(SELF const* self, size_t index) {
    return NS(SELF, try_read_from_front)(self, NS(SELF, size)(self) - 1 - index);
}

static ITEM* NS(SELF, try_write_from_front)(SELF* self, size_t index) {
    return (ITEM*)NS(SELF, try_read_from_front)(self, index);
}

static ITEM* NS(SELF, try_write_from_back)(SELF* self, size_t index) {
    return (ITEM*)NS(SELF, try_read_from_back)(self, index);
}

static void NS(SELF, push_front)(SELF* self, ITEM item) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    NS(ITEM_VECTORS, push)(&self->front, item);
    NS(SELF, rebalance)(self);
}

static void NS(SELF, push_back)(SELF* self, ITEM item) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    NS(ITEM_VECTORS, push)(&self->back, item);
    NS(SELF, rebalance)(self);
}

static ITEM NS(SELF, pop_front)(SELF* self) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    if (NS(ITEM_VECTORS, size)(&self->front) > 0) {
        ITEM result = NS(ITEM_VECTORS, pop)(&self->front);
        NS(SELF, rebalance)(self);
        return result;
    }

    ITEM result = NS(ITEM_VECTORS, pop_front)(&self->back);
    NS(SELF, rebalance)(self);
    return result;
}

static ITEM NS(SELF, pop_back)(SELF* self) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    if (NS(ITEM_VECTORS, size)(&self->back) > 0) {
        ITEM result = NS(ITEM_VECTORS, pop)(&self->back);
        NS(SELF, rebalance)(self);
        return result;
    }

    ITEM result = NS(ITEM_VECTORS, pop_front)(&self->front);
    NS(SELF, rebalance)(self);
    return result;
}

#define ITER NS(SELF, iter)
typedef ITEM* NS(ITER, item);

static bool NS(ITER, empty_item)(ITEM* const* item) { return *item == NULL; }

typedef struct {
    SELF* deque;
    size_t pos;
    mutation_version version;
} ITER;

static ITEM* NS(ITER, next)(ITER* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);
    size_t const front_size = NS(ITEM_VECTORS, size)(&iter->deque->front);
    size_t const back_size = NS(ITEM_VECTORS, size)(&iter->deque->back);
    size_t const total_size = front_size + back_size;

    if (iter->pos < total_size) {
        ITEM* item;
        if (iter->pos < front_size) {
            item = NS(ITEM_VECTORS, write)(&iter->deque->front, front_size - 1 - iter->pos);
        } else {
            item = NS(ITEM_VECTORS, write)(&iter->deque->back, iter->pos - front_size);
        }
        iter->pos++;
        return item;
    }
    return NULL;
}

static bool NS(ITER, empty)(ITER const* iter) {
    mutation_version_check(&iter->version);
    ASSUME(iter);
    return iter->pos >=
           NS(ITEM_VECTORS, size)(&iter->deque->front) + NS(ITEM_VECTORS, size)(&iter->deque->back);
}

static ITER NS(SELF, get_iter)(SELF* self) {
    ASSUME(self);
    return (ITER){.deque = self,
                  .pos = 0,
                  .version = mutation_tracker_get(&self->iterator_invalidation_tracker)};
}

#undef ITER

#define ITER_CONST NS(SELF, iter_const)
typedef ITEM const* NS(ITER_CONST, item);

static bool NS(ITER_CONST, empty_item)(ITEM const* const* item) { return *item == NULL; }

typedef struct {
    SELF const* deque;
    size_t pos;
    mutation_version version;
} ITER_CONST;

static ITEM const* NS(ITER_CONST, next)(ITER_CONST* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);
    size_t const front_size = NS(ITEM_VECTORS, size)(&iter->deque->front);
    size_t const back_size = NS(ITEM_VECTORS, size)(&iter->deque->back);
    size_t const total_size = front_size + back_size;

    if (iter->pos < total_size) {
        ITEM const* item;
        if (iter->pos < front_size) {
            item = NS(ITEM_VECTORS, read)(&iter->deque->front, front_size - 1 - iter->pos);
        } else {
            item = NS(ITEM_VECTORS, read)(&iter->deque->back, iter->pos - front_size);
        }
        iter->pos++;
        return item;
    }
    return NULL;
}

static bool NS(ITER_CONST, empty)(ITER_CONST const* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);
    return iter->pos >=
           NS(ITEM_VECTORS, size)(&iter->deque->front) + NS(ITEM_VECTORS, size)(&iter->deque->back);
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    ASSUME(self);
    return (ITER_CONST){
        .deque = self,
        .pos = 0,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

#undef ITER_CONST

static void NS(SELF, delete)(SELF* self) {
    INVARIANT_CHECK(self);
    NS(ITEM_VECTORS, delete)(&self->front);
    NS(ITEM_VECTORS, delete)(&self->back);
}

static void NS(SELF, debug)(SELF const* self, debug_fmt fmt, FILE* stream) {
    fprintf(stream, EXPAND_STRING(SELF) "@%p {\n", self);
    fmt = debug_fmt_scope_begin(fmt);
    debug_fmt_print(fmt, stream, "size: %lu,\n", NS(SELF, size)(self));

    debug_fmt_print(fmt, stream, "front: ");
    NS(ITEM_VECTORS, debug)(&self->front, fmt, stream);
    fprintf(stream, ",\n");

    debug_fmt_print(fmt, stream, "back: ");
    NS(ITEM_VECTORS, debug)(&self->back, fmt, stream);
    fprintf(stream, ",\n");

    fmt = debug_fmt_scope_end(fmt);
    debug_fmt_print(fmt, stream, "}\n");
}

#undef INVARIANT_CHECK
#undef ITEM_VECTORS
#undef ITEM_DEBUG
#undef ITEM_CLONE
#undef ITEM_DELETE
#undef ITEM

TRAIT_QUEUE(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/includes/undef.h>
