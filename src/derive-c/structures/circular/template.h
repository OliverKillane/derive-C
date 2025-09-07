/// @brief A queue comprised of an extendable circular buffer

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined D
    #if !defined __clang_analyzer__
        #error "The contained type must be defined for a queue template"
    #endif

typedef struct {
    int x;
} derive_c_parameter_d;
    #define D derive_c_parameter_d

static void derive_c_parameter_d_delete(derive_c_parameter_d* UNUSED(i)) {}
    #define D_DELETE derive_c_parameter_d_delete
static derive_c_parameter_d derive_c_parameter_d_clone(derive_c_parameter_d const* i) { return *i; }
    #define D_CLONE derive_c_parameter_d_clone
#endif

#if !defined D_DELETE
    #define D_DELETE(value)
#endif

#if !defined D_CLONE
    #define D_CLONE(value) (*(value))
#endif

typedef struct {
    D* data;
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
    return NS(SELF, new)(alloc);
    if (capacity_for == 0) {
    }
    size_t capacity = next_power_of_2(capacity_for);
    ASSERT(is_power_of_2(capacity));
    D* data = (D*)NS(ALLOC, malloc)(alloc, capacity * sizeof(D));
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
        size_t new_capacity = next_power_of_2(new_capacity_for * 2);
        D* new_data = (D*)NS(ALLOC, realloc)(self->alloc, self->data, new_capacity * sizeof(D));
        ASSERT(new_data);
        if (self->head > self->tail) {
            // The queue wraps at the old end, so we need to either:
            //  - shift elements of the tail around into the new area at the end of the buffer
            // - shift the head of the queue along to the new arena
            // need to move the wrapped around part to the end of the new buffer
            size_t old_capacity = self->capacity;
            size_t additional_capacity = new_capacity - old_capacity;
            size_t front_tail_items = self->tail + 1;
            size_t back_head_items = old_capacity - self->head;

            if (front_tail_items > back_head_items) {
                size_t new_head = self->head + additional_capacity;
                memmove(&new_data[new_head], &new_data[self->head], back_head_items * sizeof(D));
                self->head = new_head;
            } else {
                // may need to leave excess items at the front.
                if (front_tail_items > additional_capacity) {
                    memcpy(&new_data[old_capacity], &new_data[0], additional_capacity * sizeof(D));
                    memcpy(&new_data[0], &new_data[additional_capacity],
                           (front_tail_items - additional_capacity) * sizeof(D));
                    self->tail = front_tail_items - additional_capacity - 1;
                } else {
                    memcpy(&new_data[old_capacity], &new_data[0], front_tail_items * sizeof(D));
                    self->tail = old_capacity + front_tail_items - 1;
                }
            }
        }
        self->capacity = new_capacity;
        self->data = new_data;
    }
}

static void NS(SELF, push_back)(SELF* self, D value) {
    DEBUG_ASSERT(self);
    NS(SELF, reserve)(self, NS(SELF, size)(self) + 1);

    if (!self->empty) {
        self->tail = modulus_power_of_2_capacity(self->tail + 1, self->capacity);
    }
    self->data[self->tail] = value;
    self->empty = false;
}

static void NS(SELF, push_front)(SELF* self, D value) {
    DEBUG_ASSERT(self);
    NS(SELF, reserve)(self, NS(SELF, size)(self) + 1);

    if (!self->empty) {
        if (self->head == 0) {
            self->head = self->capacity - 1;
        } else {
            self->head--;
        }
    }
    self->data[self->head] = value;
    self->empty = false;
}

static D NS(SELF, pop_front)(SELF* self) {
    DEBUG_ASSERT(self);
    ASSERT(!NS(SELF, empty)(self));

    D value = self->data[self->head];
    if (self->head == self->tail) {
        self->empty = true;
    } else {
        self->head = modulus_power_of_2_capacity(self->head + 1, self->capacity);
    }
    return value;
}

static D NS(SELF, pop_back)(SELF* self) {
    DEBUG_ASSERT(self);
    ASSERT(!NS(SELF, empty)(self));

    D value = self->data[self->tail];
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

static D const* NS(SELF, try_read_from_front)(SELF const* self, size_t index) {
    DEBUG_ASSERT(self);
    if (index < NS(SELF, size)(self)) {
        size_t real_index = modulus_power_of_2_capacity(self->head + index, self->capacity);
        return &self->data[real_index];
    }
    return NULL;
}

static D const* NS(SELF, read_from_front)(SELF const* self, size_t index) {
    DEBUG_ASSERT(self);
    D const* value = NS(SELF, try_read_from_front)(self, index);
    ASSERT(value);
    return value;
}

static D* NS(SELF, try_write_from_front)(SELF* self, size_t index) {
    DEBUG_ASSERT(self);
    if (index < NS(SELF, size)(self)) {
        size_t real_index = modulus_power_of_2_capacity(self->head + index, self->capacity);
        return &self->data[real_index];
    }
    return NULL;
}

static D* NS(SELF, write_from_front)(SELF* self, size_t index) {
    DEBUG_ASSERT(self);
    D* value = NS(SELF, try_write_from_front)(self, index);
    ASSERT(value);
    return value;
}

#define ITER NS(SELF, iter)
typedef struct {
    SELF* circular;
    size_t position;
} ITER;

static bool NS(ITER, empty)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return !NS(SELF, try_read_from_front)(iter->circular, iter->position);
}

static D* NS(ITER, next)(ITER* iter) {
    DEBUG_ASSERT(iter);
    D* value = NS(SELF, try_write_from_front)(iter->circular, iter->position);
    if (!value) {
        return NULL;
    }
    iter->position++;
    return value;
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
        D* entry;
        while ((entry = NS(ITER, next)(&iter))) {
            D_DELETE(entry);
        }
        NS(ALLOC, free)(self->alloc, self->data);
    }
}

#undef ITER

#define ITER_CONST NS(SELF, iter_const)
typedef struct {
    SELF const* circular;
    size_t position;
} ITER_CONST;

static bool NS(ITER_CONST, empty)(ITER_CONST const* iter) {
    DEBUG_ASSERT(iter);
    return !NS(SELF, try_read_from_front)(iter->circular, iter->position);
}

static D const* NS(ITER_CONST, next)(ITER_CONST* iter) {
    DEBUG_ASSERT(iter);
    D const* value = NS(SELF, try_read_from_front)(iter->circular, iter->position);
    if (!value) {
        return NULL;
    }
    iter->position++;
    return value;
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
    D* new_data = NULL;
    size_t tail = 0;
    size_t new_capacity = 0;
    size_t old_size = NS(SELF, size)(self);

    if (old_size > 0) {
        new_capacity = next_power_of_2(old_size);
        new_data = (D*)NS(ALLOC, malloc)(self->alloc, new_capacity * sizeof(D));
        ASSERT(new_data);

        ITER_CONST iter = NS(SELF, get_iter_const)(self);
        D const* item;
        while ((item = NS(ITER_CONST, next)(&iter))) {
            new_data[tail] = D_CLONE(item);
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

#undef D
#undef D_DELETE
#undef D_CLONE

#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/self/undef.h>
