/// @brief A queue comprised of an extendable circular buffer

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <derive-c/container/queue/trait.h>
#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>
#include <derive-c/core/placeholder.h>

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined ITEM
    #if !defined PLACEHOLDERS
        #error "The contained type must be defined for a queue template"
    #endif

typedef struct {
    int x;
} item_t;
    #define ITEM item_t
static void item_delete(item_t* UNUSED(self)) {}
    #define ITEM_DELETE item_delete
static item_t item_clone(item_t const* self) { return *self; }
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

typedef struct {
    ITEM* data;
    size_t capacity;
    size_t head; /* Index of the first element */
    size_t tail; /* Index of the last element */
    bool empty;  /* Used to differentiate between head == tail when 1 element, or empty */
    ALLOC* alloc;
    gdb_marker derive_c_circular;
} SELF;

static SELF NS(SELF, new)(ALLOC* alloc) {
    return (SELF){.data = NULL,
                  .head = 0,
                  .tail = 0,
                  .empty = true,
                  .alloc = alloc,
                  .derive_c_circular = (gdb_marker){}};
}

static SELF NS(SELF, new_with_capacity_for)(size_t capacity_for, ALLOC* alloc) {
    if (capacity_for == 0) {
        return NS(SELF, new)(alloc);
    }
    size_t const capacity = next_power_of_2(capacity_for);
    ASSERT(is_power_of_2(capacity));
    ITEM* data = (ITEM*)NS(ALLOC, malloc)(alloc, capacity * sizeof(ITEM));
    ASSERT(data);

    return (SELF){
        .data = data,
        .capacity = capacity,
        .head = 0,
        .tail = 0,
        .empty = true,
        .alloc = alloc,
        .derive_c_circular = (gdb_marker){},
    };
}

static bool NS(SELF, empty)(SELF const* self) {
    DEBUG_ASSERT(self);
    if (self->empty) {
        DEBUG_ASSERT(self->head == self->tail);
    }
    return self->empty;
}

static size_t NS(SELF, size)(SELF const* self) {
    DEBUG_ASSERT(self);
    if (self->empty) {
        DEBUG_ASSERT(self->tail == self->head);
        return 0;
    }

    if (self->head <= self->tail) {
        return (self->tail - self->head) + 1;
    }
    return (self->capacity - self->head) + self->tail + 1;
}

static void NS(SELF, reserve)(SELF* self, size_t new_capacity_for) {
    DEBUG_ASSERT(self);

    if (new_capacity_for > self->capacity) {
        size_t const new_capacity = next_power_of_2(new_capacity_for * 2);
        ITEM* new_data =
            (ITEM*)NS(ALLOC, realloc)(self->alloc, self->data, new_capacity * sizeof(ITEM));
        ASSERT(new_data);
        if (self->head > self->tail) {
            // The queue wraps at the old end, so we need to either:
            //  - shift elements of the tail around into the new area at the end of the buffer
            // - shift the head of the queue along to the new arena
            // need to move the wrapped around part to the end of the new buffer
            size_t const old_capacity = self->capacity;
            size_t const additional_capacity = new_capacity - old_capacity;
            size_t const front_tail_items = self->tail + 1;
            size_t const back_head_items = old_capacity - self->head;

            if (front_tail_items > back_head_items) {
                size_t const new_head = self->head + additional_capacity;
                memmove(&new_data[new_head], &new_data[self->head], back_head_items * sizeof(ITEM));
                self->head = new_head;
            } else {
                // as we go to the next power of 2 each time, the additional capacity is always >=
                // the number of items at the front. As a result, we never need to consider:
                // - Only copying the first n from the front
                // - Shifting some of the front items to the start of the buffer
                DEBUG_ASSERT(front_tail_items <= additional_capacity);

                memcpy(&new_data[old_capacity], &new_data[0], front_tail_items * sizeof(ITEM));
                self->tail = old_capacity + front_tail_items - 1;
            }
        }
        self->capacity = new_capacity;
        self->data = new_data;
    }
}

static void NS(SELF, push_back)(SELF* self, ITEM item) {
    DEBUG_ASSERT(self);
    NS(SELF, reserve)(self, NS(SELF, size)(self) + 1);

    if (!self->empty) {
        self->tail = modulus_power_of_2_capacity(self->tail + 1, self->capacity);
    }
    self->data[self->tail] = item;
    self->empty = false;
}

static void NS(SELF, push_front)(SELF* self, ITEM item) {
    DEBUG_ASSERT(self);
    NS(SELF, reserve)(self, NS(SELF, size)(self) + 1);

    if (!self->empty) {
        if (self->head == 0) {
            self->head = self->capacity - 1;
        } else {
            self->head--;
        }
    }
    self->data[self->head] = item;
    self->empty = false;
}

static ITEM NS(SELF, pop_front)(SELF* self) {
    DEBUG_ASSERT(self);
    ASSERT(!NS(SELF, empty)(self));

    ITEM value = self->data[self->head];
    if (self->head == self->tail) {
        self->empty = true;
    } else {
        self->head = modulus_power_of_2_capacity(self->head + 1, self->capacity);
    }
    return value;
}

static ITEM NS(SELF, pop_back)(SELF* self) {
    DEBUG_ASSERT(self);
    ASSERT(!NS(SELF, empty)(self));

    ITEM value = self->data[self->tail];
    if (self->head == self->tail) {
        self->empty = true;
    } else {
        if (self->tail == 0) {
            self->tail = self->capacity - 1;
        } else {
            self->tail--;
        }
    }
    return value;
}

static ITEM const* NS(SELF, try_read_from_front)(SELF const* self, size_t index) {
    DEBUG_ASSERT(self);
    if (index < NS(SELF, size)(self)) {
        size_t const real_index = modulus_power_of_2_capacity(self->head + index, self->capacity);
        return &self->data[real_index];
    }
    return NULL;
}

static ITEM const* NS(SELF, read_from_front)(SELF const* self, size_t index) {
    DEBUG_ASSERT(self);
    ITEM const* item = NS(SELF, try_read_from_front)(self, index);
    ASSERT(item);
    return item;
}

static ITEM* NS(SELF, try_write_from_front)(SELF* self, size_t index) {
    DEBUG_ASSERT(self);
    if (index < NS(SELF, size)(self)) {
        size_t const real_index = modulus_power_of_2_capacity(self->head + index, self->capacity);
        return &self->data[real_index];
    }
    return NULL;
}

static ITEM* NS(SELF, write_from_front)(SELF* self, size_t index) {
    DEBUG_ASSERT(self);
    ITEM* value = NS(SELF, try_write_from_front)(self, index);
    ASSERT(value);
    return value;
}

#define ITER NS(SELF, iter)
typedef ITEM* NS(ITER, item);

typedef struct {
    SELF* circular;
    size_t position;
} ITER;

static bool NS(ITER, empty)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return !NS(SELF, try_read_from_front)(iter->circular, iter->position);
}

static ITEM* NS(ITER, next)(ITER* iter) {
    DEBUG_ASSERT(iter);
    ITEM* item = NS(SELF, try_write_from_front)(iter->circular, iter->position);
    if (!item) {
        return NULL;
    }
    iter->position++;
    return item;
}

static ITER NS(SELF, get_iter)(SELF* self) {
    DEBUG_ASSERT(self);
    return (ITER){
        .circular = self,
        .position = 0,
    };
}

static void NS(SELF, delete)(SELF* self) {
    DEBUG_ASSERT(self);
    if (self->data) {
        ITER iter = NS(SELF, get_iter)(self);
        ITEM* item;
        while ((item = NS(ITER, next)(&iter))) {
            ITEM_DELETE(item);
        }
        NS(ALLOC, free)(self->alloc, self->data);
    }
}

#undef ITER

#define ITER_CONST NS(SELF, iter_const)
typedef ITEM const* NS(ITER_CONST, item);

typedef struct {
    SELF const* circular;
    size_t position;
} ITER_CONST;

static bool NS(ITER_CONST, empty)(ITER_CONST const* iter) {
    DEBUG_ASSERT(iter);
    return !NS(SELF, try_read_from_front)(iter->circular, iter->position);
}

static ITEM const* NS(ITER_CONST, next)(ITER_CONST* iter) {
    DEBUG_ASSERT(iter);
    ITEM const* item = NS(SELF, try_read_from_front)(iter->circular, iter->position);
    if (!item) {
        return NULL;
    }
    iter->position++;
    return item;
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    DEBUG_ASSERT(self);
    return (ITER_CONST){
        .circular = self,
        .position = 0,
    };
}

static SELF NS(SELF, clone)(SELF const* self) {
    DEBUG_ASSERT(self);
    ITEM* new_data = NULL;
    size_t tail = 0;
    size_t new_capacity = 0;
    size_t const old_size = NS(SELF, size)(self);

    if (old_size > 0) {
        new_capacity = next_power_of_2(old_size);
        new_data = (ITEM*)NS(ALLOC, malloc)(self->alloc, new_capacity * sizeof(ITEM));
        ASSERT(new_data);

        ITER_CONST iter = NS(SELF, get_iter_const)(self);
        ITEM const* item;
        while ((item = NS(ITER_CONST, next)(&iter))) {
            new_data[tail] = ITEM_CLONE(item);
            tail++;
        }
        tail--;
    }
    return (SELF){
        .data = new_data,
        .capacity = new_capacity,
        .head = 0,
        .tail = tail,
        .empty = self->empty,
        .alloc = self->alloc,
        .derive_c_circular = (gdb_marker){},
    };
}

#undef ITER_CONST

#undef ITEM
#undef ITEM_DELETE
#undef ITEM_CLONE

#include <derive-c/core/alloc/undef.h>
TRAIT_QUEUE(SELF);

#include <derive-c/core/self/undef.h>
