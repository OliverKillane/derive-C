/// @brief A simple double ended queue

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "utils.h"

#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined I
    #if !defined __clang_analyzer__
        #error "The contained type must be defined for a vector template"
    #endif

typedef struct {
    int x;
} derive_c_parameter_i;
    #define I derive_c_parameter_i

static void derive_c_parameter_t_delete(derive_c_parameter_i* UNUSED(i)) {}
    #define I_DELETE derive_c_parameter_t_delete
#endif

#if !defined I_DELETE
    #define I_DELETE(value)
#endif

#define ITEM_VECTORS NS(NAME, item_vectors)

#pragma push_macro("ALLOC")

#define T I
#define T_DELETE I_DELETE
#define INTERNAL_NAME ITEM_VECTORS
#include <derive-c/structures/vector/template.h>

#pragma pop_macro("ALLOC")

typedef struct {
    ITEM_VECTORS front;
    ITEM_VECTORS back;
    ALLOC* alloc;
    gdb_marker derive_c_dequeue;
} SELF;

static SELF NS(SELF, new)(ALLOC* alloc) {
    return (SELF){.front = NS(ITEM_VECTORS, new)(alloc),
                  .back = NS(ITEM_VECTORS, new)(alloc),
                  .alloc = alloc,
                  .derive_c_dequeue = (gdb_marker){}};
}

static SELF NS(SELF, new_with_capacity)(size_t front_and_back_capacity, ALLOC* alloc) {
    return (SELF){.front = NS(ITEM_VECTORS, new_with_capacity)(front_and_back_capacity, alloc),
                  .back = NS(ITEM_VECTORS, new_with_capacity)(front_and_back_capacity, alloc),
                  .alloc = alloc,
                  .derive_c_dequeue = (gdb_marker){}};
}

static SELF NS(SELF, shallow_clone)(SELF const* other) {
    DEBUG_ASSERT(other);
    return (SELF){.front = NS(ITEM_VECTORS, shallow_clone)(&other->front),
                  .back = NS(ITEM_VECTORS, shallow_clone)(&other->back),
                  .alloc = other->alloc,
                  .derive_c_dequeue = (gdb_marker){}};
}

static void NS(SELF, rebalance)(SELF* self) {
    DEBUG_ASSERT(self);

    size_t front_size = NS(ITEM_VECTORS, size)(&self->front);
    size_t back_size = NS(ITEM_VECTORS, size)(&self->back);
    size_t total_size = front_size + back_size;

    if (!dequeue_rebalance_policy(total_size, front_size)) {
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
    } else if (back_size > front_size + 1) {
        source = &self->back;
        target = &self->front;
        source_size = back_size;
        target_size = front_size;
    } else {
        return;
    }

    size_t to_move = (source_size - target_size) / 2;

    NS(ITEM_VECTORS, reserve)(target, target_size + to_move);

    I* source_data = NS(ITEM_VECTORS, data)(source);
    I* target_data = NS(ITEM_VECTORS, data)(target);

    memmove(&target_data[to_move], target_data, target_size * sizeof(I));

    for (size_t i = 0; i < to_move; i++) {
        target_data[to_move - 1 - i] = source_data[i];
    }

    memmove(source_data, &source_data[to_move], (source_size - to_move) * sizeof(I));

    target->size += to_move;
    source->size -= to_move;
}

static I const* NS(SELF, peek_front)(SELF const* self) {
    DEBUG_ASSERT(self);

    size_t front_size = NS(ITEM_VECTORS, size)(&self->front);
    if (front_size > 0) {
        return NS(ITEM_VECTORS, read)(&self->front, front_size - 1);
    }

    return NS(ITEM_VECTORS, try_read)(&self->back, 0);
}

static I const* NS(SELF, peek_back)(SELF const* self) {
    DEBUG_ASSERT(self);

    size_t back_size = NS(ITEM_VECTORS, size)(&self->back);
    if (back_size > 0) {
        return NS(ITEM_VECTORS, read)(&self->back, back_size - 1);
    }

    return NS(ITEM_VECTORS, try_read)(&self->front, 0);
}

static void NS(SELF, push_front)(SELF* self, I item) {
    DEBUG_ASSERT(self);
    NS(ITEM_VECTORS, push)(&self->front, item);
    NS(SELF, rebalance)(self);
}

static void NS(SELF, push_back)(SELF* self, I item) {
    DEBUG_ASSERT(self);
    NS(ITEM_VECTORS, push)(&self->back, item);
    NS(SELF, rebalance)(self);
}

static I NS(SELF, pop_front)(SELF* self) {
    DEBUG_ASSERT(self);
    if (NS(ITEM_VECTORS, size)(&self->front) > 0) {
        I result = NS(ITEM_VECTORS, pop)(&self->front);
        NS(SELF, rebalance)(self);
        return result;
    }

    I result = NS(ITEM_VECTORS, pop_front)(&self->back);
    NS(SELF, rebalance)(self);
    return result;
}

static I NS(SELF, pop_back)(SELF* self) {
    DEBUG_ASSERT(self);
    if (NS(ITEM_VECTORS, size)(&self->back) > 0) {
        I result = NS(ITEM_VECTORS, pop)(&self->back);
        NS(SELF, rebalance)(self);
        return result;
    }

    I result = NS(ITEM_VECTORS, pop_front)(&self->front);
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

typedef struct {
    SELF* dequeue;
    size_t pos;
} ITER;

static I* NS(ITER, next)(ITER* iter) {
    DEBUG_ASSERT(iter);
    size_t front_size = NS(ITEM_VECTORS, size)(&iter->dequeue->front);
    size_t back_size = NS(ITEM_VECTORS, size)(&iter->dequeue->back);
    size_t total_size = front_size + back_size;

    if (iter->pos < total_size) {
        I* item;
        if (iter->pos < front_size) {
            item = NS(ITEM_VECTORS, write)(&iter->dequeue->front, front_size - 1 - iter->pos);
        } else {
            item = NS(ITEM_VECTORS, write)(&iter->dequeue->back, iter->pos - front_size);
        }
        iter->pos++;
        return item;
    }
    return NULL;
}

static bool NS(ITER, empty)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos >= NS(ITEM_VECTORS, size)(&iter->dequeue->front) +
                            NS(ITEM_VECTORS, size)(&iter->dequeue->back);
}

static ITER NS(SELF, get_iter)(SELF* self) {
    DEBUG_ASSERT(self);
    return (ITER){
        .dequeue = self,
        .pos = 0,
    };
}

#undef ITER

#define ITER_CONST NS(SELF, iter_const)

typedef struct {
    SELF const* dequeue;
    size_t pos;
} ITER_CONST;

static I const* NS(ITER_CONST, next)(ITER_CONST* iter) {
    DEBUG_ASSERT(iter);
    size_t front_size = NS(ITEM_VECTORS, size)(&iter->dequeue->front);
    size_t back_size = NS(ITEM_VECTORS, size)(&iter->dequeue->back);
    size_t total_size = front_size + back_size;

    if (iter->pos < total_size) {
        I const* item;
        if (iter->pos < front_size) {
            item = NS(ITEM_VECTORS, read)(&iter->dequeue->front, front_size - 1 - iter->pos);
        } else {
            item = NS(ITEM_VECTORS, read)(&iter->dequeue->back, iter->pos - front_size);
        }
        iter->pos++;
        return item;
    }
    return NULL;
}

static bool NS(ITER_CONST, empty)(ITER_CONST const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos >= NS(ITEM_VECTORS, size)(&iter->dequeue->front) +
                            NS(ITEM_VECTORS, size)(&iter->dequeue->back);
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    DEBUG_ASSERT(self);
    return (ITER_CONST){
        .dequeue = self,
        .pos = 0,
    };
}

#undef ITER_CONST

static void NS(SELF, delete)(SELF* self) {
    DEBUG_ASSERT(self);
    NS(ITEM_VECTORS, delete)(&self->front);
    NS(ITEM_VECTORS, delete)(&self->back);
}

#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/self/undef.h>
