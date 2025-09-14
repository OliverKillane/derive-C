/// @brief A simple double ended queue

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "utils.h"

#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined A
    #if !defined __clang_analyzer__
        #error "The contained type must be defined for a deque template"
    #endif

typedef struct {
    int x;
} derive_c_parameter_a;
    #define A derive_c_parameter_a

static void derive_c_parameter_t_delete(derive_c_parameter_a* UNUSED(a)) {}
    #define A_DELETE derive_c_parameter_t_delete
static derive_c_parameter_a derive_c_parameter_a_clone(derive_c_parameter_a const* a) { return *a; }
    #define A_CLONE derive_c_parameter_a_clone
#endif

#if !defined A_DELETE
    #define A_DELETE(value)
#endif

#if !defined A_CLONE
    #define A_CLONE(value) (*(value))
#endif

#define ITEM_VECTORS NS(NAME, item_vectors)

#pragma push_macro("ALLOC")

#define T A
#define T_DELETE A_DELETE
#define T_CLONE A_CLONE
#define INTERNAL_NAME ITEM_VECTORS
#include <derive-c/structures/vector/template.h>

#pragma pop_macro("ALLOC")

typedef struct {
    ITEM_VECTORS front;
    ITEM_VECTORS back;
    ALLOC* alloc;
    gdb_marker derive_c_deque;
} SELF;

static SELF NS(SELF, new)(ALLOC* alloc) {
    return (SELF){.front = NS(ITEM_VECTORS, new)(alloc),
                  .back = NS(ITEM_VECTORS, new)(alloc),
                  .alloc = alloc,
                  .derive_c_deque = (gdb_marker){}};
}

static SELF NS(SELF, new_with_capacity)(size_t front_and_back_capacity, ALLOC* alloc) {
    return (SELF){.front = NS(ITEM_VECTORS, new_with_capacity)(front_and_back_capacity, alloc),
                  .back = NS(ITEM_VECTORS, new_with_capacity)(front_and_back_capacity, alloc),
                  .alloc = alloc,
                  .derive_c_deque = (gdb_marker){}};
}

static SELF NS(SELF, clone)(SELF const* other) {
    DEBUG_ASSERT(other);
    return (SELF){.front = NS(ITEM_VECTORS, clone)(&other->front),
                  .back = NS(ITEM_VECTORS, clone)(&other->back),
                  .alloc = other->alloc,
                  .derive_c_deque = (gdb_marker){}};
}

static void NS(SELF, rebalance)(SELF* self) {
    DEBUG_ASSERT(self);

    size_t front_size = NS(ITEM_VECTORS, size)(&self->front);
    size_t back_size = NS(ITEM_VECTORS, size)(&self->back);
    size_t total_size = front_size + back_size;

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

    size_t to_move = (source_size - target_size) / 2;

    NS(ITEM_VECTORS, reserve)(target, target_size + to_move);

    A* source_data = NS(ITEM_VECTORS, data)(source);
    A* target_data = NS(ITEM_VECTORS, data)(target);

    memmove(&target_data[to_move], target_data, target_size * sizeof(A));

    for (size_t i = 0; i < to_move; i++) {
        target_data[to_move - 1 - i] = source_data[i];
    }

    memmove(source_data, &source_data[to_move], (source_size - to_move) * sizeof(A));

    target->size += to_move;
    source->size -= to_move;
}

static A const* NS(SELF, peek_front_read)(SELF const* self) {
    DEBUG_ASSERT(self);

    size_t front_size = NS(ITEM_VECTORS, size)(&self->front);
    if (front_size > 0) {
        return NS(ITEM_VECTORS, read)(&self->front, front_size - 1);
    }

    return NS(ITEM_VECTORS, try_read)(&self->back, 0);
}

static A* NS(SELF, peek_front_write)(SELF* self) {
    DEBUG_ASSERT(self);

    size_t front_size = NS(ITEM_VECTORS, size)(&self->front);
    if (front_size > 0) {
        return NS(ITEM_VECTORS, write)(&self->front, front_size - 1);
    }

    return NS(ITEM_VECTORS, try_write)(&self->back, 0);
}

static A const* NS(SELF, peek_back_read)(SELF const* self) {
    DEBUG_ASSERT(self);

    size_t back_size = NS(ITEM_VECTORS, size)(&self->back);
    if (back_size > 0) {
        return NS(ITEM_VECTORS, read)(&self->back, back_size - 1);
    }

    return NS(ITEM_VECTORS, try_read)(&self->front, 0);
}

static A* NS(SELF, peek_back_write)(SELF* self) {
    DEBUG_ASSERT(self);

    size_t back_size = NS(ITEM_VECTORS, size)(&self->back);
    if (back_size > 0) {
        return NS(ITEM_VECTORS, write)(&self->back, back_size - 1);
    }

    return NS(ITEM_VECTORS, try_write)(&self->front, 0);
}

static void NS(SELF, push_front)(SELF* self, A item) {
    DEBUG_ASSERT(self);
    NS(ITEM_VECTORS, push)(&self->front, item);
    NS(SELF, rebalance)(self);
}

static void NS(SELF, push_back)(SELF* self, A item) {
    DEBUG_ASSERT(self);
    NS(ITEM_VECTORS, push)(&self->back, item);
    NS(SELF, rebalance)(self);
}

static A NS(SELF, pop_front)(SELF* self) {
    DEBUG_ASSERT(self);
    if (NS(ITEM_VECTORS, size)(&self->front) > 0) {
        A result = NS(ITEM_VECTORS, pop)(&self->front);
        NS(SELF, rebalance)(self);
        return result;
    }

    A result = NS(ITEM_VECTORS, pop_front)(&self->back);
    NS(SELF, rebalance)(self);
    return result;
}

static A NS(SELF, pop_back)(SELF* self) {
    DEBUG_ASSERT(self);
    if (NS(ITEM_VECTORS, size)(&self->back) > 0) {
        A result = NS(ITEM_VECTORS, pop)(&self->back);
        NS(SELF, rebalance)(self);
        return result;
    }

    A result = NS(ITEM_VECTORS, pop_front)(&self->front);
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
    SELF* deque;
    size_t pos;
} ITER;

static A* NS(ITER, next)(ITER* iter) {
    DEBUG_ASSERT(iter);
    size_t front_size = NS(ITEM_VECTORS, size)(&iter->deque->front);
    size_t back_size = NS(ITEM_VECTORS, size)(&iter->deque->back);
    size_t total_size = front_size + back_size;

    if (iter->pos < total_size) {
        A* item;
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
    DEBUG_ASSERT(iter);
    return iter->pos >=
           NS(ITEM_VECTORS, size)(&iter->deque->front) + NS(ITEM_VECTORS, size)(&iter->deque->back);
}

static ITER NS(SELF, get_iter)(SELF* self) {
    DEBUG_ASSERT(self);
    return (ITER){
        .deque = self,
        .pos = 0,
    };
}

#undef ITER

#define ITER_CONST NS(SELF, iter_const)

typedef struct {
    SELF const* deque;
    size_t pos;
} ITER_CONST;

static A const* NS(ITER_CONST, next)(ITER_CONST* iter) {
    DEBUG_ASSERT(iter);
    size_t front_size = NS(ITEM_VECTORS, size)(&iter->deque->front);
    size_t back_size = NS(ITEM_VECTORS, size)(&iter->deque->back);
    size_t total_size = front_size + back_size;

    if (iter->pos < total_size) {
        A const* item;
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
    return iter->pos >=
           NS(ITEM_VECTORS, size)(&iter->deque->front) + NS(ITEM_VECTORS, size)(&iter->deque->back);
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    DEBUG_ASSERT(self);
    return (ITER_CONST){
        .deque = self,
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
