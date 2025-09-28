/// @brief A simple double ended queue

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "derive-c/core/debug/mutation_tracker.h"
#include "utils.h"

#include <derive-c/container/queue/trait.h>
#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>
#include <derive-c/core/placeholder.h>

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined ITEM
    #if !defined PLACEHOLDERS
        #error "The contained type must be defined for a deque template"
    #endif

typedef struct {
    int x;
} item_t;
    #define ITEM item_t

static void item_delete(item_t* UNUSED(a)) {}
    #define ITEM_DELETE item_delete
static item_t item_clone(item_t const* a) { return *a; }
    #define ITEM_CLONE item_clone
#endif

#if !defined ITEM_DELETE
    #define ITEM_DELETE(value)
#endif

#if !defined ITEM_CLONE
    #define ITEM_CLONE(value) (*(value))
#endif

typedef ITEM NS(SELF, item_t);
typedef ALLOC NS(SELF, alloc_t);

#define ITEM_VECTORS NS(NAME, item_vectors)

#pragma push_macro("ALLOC")
#pragma push_macro("ITEM")
#pragma push_macro("ITEM_CLONE")
#pragma push_macro("ITEM_DELETE")

// ITEM is already defined
#define INTERNAL_NAME ITEM_VECTORS
#include <derive-c/container/vector/dynamic/template.h>

#pragma pop_macro("ALLOC")
#pragma pop_macro("ITEM")
#pragma pop_macro("ITEM_CLONE")
#pragma pop_macro("ITEM_DELETE")

typedef struct {
    ITEM_VECTORS front;
    ITEM_VECTORS back;
    ALLOC* alloc;
    gdb_marker derive_c_deque;
    mutation_tracker iterator_invalidation_tracker;
} SELF;

static SELF NS(SELF, new)(ALLOC* alloc) {
    return (SELF){
        .front = NS(ITEM_VECTORS, new)(alloc),
        .back = NS(ITEM_VECTORS, new)(alloc),
        .alloc = alloc,
        .derive_c_deque = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static SELF NS(SELF, new_with_capacity)(size_t front_and_back_capacity, ALLOC* alloc) {
    return (SELF){
        .front = NS(ITEM_VECTORS, new_with_capacity)(front_and_back_capacity, alloc),
        .back = NS(ITEM_VECTORS, new_with_capacity)(front_and_back_capacity, alloc),
        .alloc = alloc,
        .derive_c_deque = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static SELF NS(SELF, clone)(SELF const* other) {
    DEBUG_ASSERT(other);
    return (SELF){
        .front = NS(ITEM_VECTORS, clone)(&other->front),
        .back = NS(ITEM_VECTORS, clone)(&other->back),
        .alloc = other->alloc,
        .derive_c_deque = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static void NS(SELF, rebalance)(SELF* self) {
    DEBUG_ASSERT(self);
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
        DEBUG_ASSERT(back_size > front_size + 1)
        source = &self->back;
        target = &self->front;
        source_size = back_size;
        target_size = front_size;
    }

    size_t const to_move = (source_size - target_size) / 2;
    NS(ITEM_VECTORS, transfer_reverse)(source, target, to_move);
}

static ITEM const* NS(SELF, peek_front_read)(SELF const* self) {
    DEBUG_ASSERT(self);

    size_t const front_size = NS(ITEM_VECTORS, size)(&self->front);
    if (front_size > 0) {
        return NS(ITEM_VECTORS, read)(&self->front, front_size - 1);
    }

    return NS(ITEM_VECTORS, try_read)(&self->back, 0);
}

static ITEM* NS(SELF, peek_front_write)(SELF* self) {
    DEBUG_ASSERT(self);

    size_t const front_size = NS(ITEM_VECTORS, size)(&self->front);
    if (front_size > 0) {
        return NS(ITEM_VECTORS, write)(&self->front, front_size - 1);
    }

    return NS(ITEM_VECTORS, try_write)(&self->back, 0);
}

static ITEM const* NS(SELF, peek_back_read)(SELF const* self) {
    DEBUG_ASSERT(self);

    size_t const back_size = NS(ITEM_VECTORS, size)(&self->back);
    if (back_size > 0) {
        return NS(ITEM_VECTORS, read)(&self->back, back_size - 1);
    }

    return NS(ITEM_VECTORS, try_read)(&self->front, 0);
}

static ITEM* NS(SELF, peek_back_write)(SELF* self) {
    DEBUG_ASSERT(self);

    size_t const back_size = NS(ITEM_VECTORS, size)(&self->back);
    if (back_size > 0) {
        return NS(ITEM_VECTORS, write)(&self->back, back_size - 1);
    }

    return NS(ITEM_VECTORS, try_write)(&self->front, 0);
}

static void NS(SELF, push_front)(SELF* self, ITEM item) {
    DEBUG_ASSERT(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    NS(ITEM_VECTORS, push)(&self->front, item);
    NS(SELF, rebalance)(self);
}

static void NS(SELF, push_back)(SELF* self, ITEM item) {
    DEBUG_ASSERT(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    NS(ITEM_VECTORS, push)(&self->back, item);
    NS(SELF, rebalance)(self);
}

static ITEM NS(SELF, pop_front)(SELF* self) {
    DEBUG_ASSERT(self);
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
    DEBUG_ASSERT(self);
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

static size_t NS(SELF, size)(SELF const* self) {
    DEBUG_ASSERT(self);
    return NS(ITEM_VECTORS, size)(&self->front) + NS(ITEM_VECTORS, size)(&self->back);
}

static bool NS(SELF, empty)(SELF const* self) {
    DEBUG_ASSERT(self);
    return NS(ITEM_VECTORS, size)(&self->front) == 0 && NS(ITEM_VECTORS, size)(&self->back) == 0;
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
    DEBUG_ASSERT(iter);
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
    DEBUG_ASSERT(iter);
    return iter->pos >=
           NS(ITEM_VECTORS, size)(&iter->deque->front) + NS(ITEM_VECTORS, size)(&iter->deque->back);
}

static ITER NS(SELF, get_iter)(SELF* self) {
    DEBUG_ASSERT(self);
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
    DEBUG_ASSERT(iter);
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
    DEBUG_ASSERT(iter);
    mutation_version_check(&iter->version);
    return iter->pos >=
           NS(ITEM_VECTORS, size)(&iter->deque->front) + NS(ITEM_VECTORS, size)(&iter->deque->back);
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    DEBUG_ASSERT(self);
    return (ITER_CONST){
        .deque = self,
        .pos = 0,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

#undef ITER_CONST

static void NS(SELF, delete)(SELF* self) {
    DEBUG_ASSERT(self);
    NS(ITEM_VECTORS, delete)(&self->front);
    NS(ITEM_VECTORS, delete)(&self->back);
}

#include <derive-c/core/alloc/undef.h>
TRAIT_QUEUE(SELF);

#include <derive-c/core/self/undef.h>
