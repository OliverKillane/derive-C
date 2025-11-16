/// @brief A vector-backed arena, with support for small indices.


#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include <derive-c/container/arena/contiguous/includes.h>
#endif


#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined INDEX_BITS
    #if !defined PLACEHOLDERS
TEMPLATE_ERROR("The number of bits (8,16,32,64) to use for the arena's key")
    #endif
    #define INDEX_BITS 32
#endif

#if !defined VALUE
    #if !defined PLACEHOLDERS
TEMPLATE_ERROR("The value type to place in the arena must be defined")
    #endif
typedef struct {
    int x;
} value_t;
    #define VALUE value_t
static void value_delete(value_t* self) { (void)self; }
    #define VALUE_DELETE value_delete
static value_t value_clone(value_t const* self) { return *self; }
    #define VALUE_CLONE value_clone
#endif

STATIC_ASSERT(sizeof(VALUE), "VALUE must be a non-zero sized type");

#if !defined VALUE_DELETE
    #define VALUE_DELETE NO_DELETE
#endif

#if !defined VALUE_CLONE
    #define VALUE_CLONE COPY_CLONE
#endif

#if !defined VALUE_DEBUG
    #define VALUE_DEBUG DEFAULT_DEBUG
#endif

#include <derive-c/core/index/def.h>

#define SLOT NS(SELF, slot)

#define CHECK_ACCESS_INDEX(self, index) ((index).index < (self)->exclusive_end)

// JUSTIFY: Macro rather than static
//           - Avoids the need to cast to the INDEX_TYPE
#define RESIZE_FACTOR 2

// INVARIANT: < CAPACITY_EXCLUSIVE_UPPER
#define INDEX NS(SELF, index_t)

typedef VALUE NS(SELF, value_t);
typedef ALLOC NS(SELF, alloc_t);

typedef struct {
    INDEX_TYPE index;
} INDEX;

static bool NS(INDEX, eq)(INDEX const* idx_1, INDEX const* idx_2) {
    return idx_1->index == idx_2->index;
}

static void NS(INDEX, debug)(INDEX const* idx, debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, EXPAND_STRING(INDEX) " { %lu }", (size_t)idx->index);
}

typedef struct {
    union {
        VALUE value;
        INDEX_TYPE next_free;
    };

    // JUSTIFY: Present flag last
    //           - Reduces size, C ABI orders fields, placing it before a larger
    //             (aligned) value would add (alignment - 1 byte) of unecessary
    //             padding
    bool present;
} SLOT;

static void NS(SLOT, memory_tracker_empty)(SLOT const* slot) {
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE, &slot->value,
                       sizeof(VALUE));
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_WRITE, &slot->next_free,
                       sizeof(INDEX_TYPE));
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_READ_WRITE, &slot->present,
                       sizeof(bool));
}

static void NS(SLOT, memory_tracker_present)(SLOT const* slot) {
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE, &slot->next_free,
                       sizeof(INDEX_TYPE));
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_WRITE, &slot->value,
                       sizeof(VALUE));
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_READ_WRITE, &slot->present,
                       sizeof(bool));
}

typedef struct {
    SLOT* slots;
    size_t capacity;
    INDEX_TYPE free_list;

    // JUSTIFY: Separately recording the first_uninit_entry
    //           - Allows us to not use calloc for the buffer (for < first_uninit_entry
    //             we can check represent, for >= first_uninit_entry we know none are
    //             present so having uninitialised entries is fine)
    size_t exclusive_end;

    // INVARIANT: If free_list == EMPTY_INDEX, then all values from [0, count)
    //            are present
    size_t count;

    ALLOC* alloc;
    gdb_marker derive_c_arena_basic;
    mutation_tracker iterator_invalidation_tracker;
} SELF;

#define INVARIANT_CHECK(self)                                                                      \
    ASSUME(self);                                                                                  \
    ASSUME((self)->count <= (self)->capacity);                                                     \
    ASSUME((self)->exclusive_end >= (self)->count);                                                \
    ASSUME((self)->count <= MAX_INDEX);

static SELF NS(SELF, new_with_capacity_for)(INDEX_TYPE items, ALLOC* alloc) {
    ASSERT(items > 0);
    size_t capacity = next_power_of_2(items);
    ASSERT(capacity <= CAPACITY_EXCLUSIVE_UPPER);
    SLOT* slots = (SLOT*)NS(ALLOC, calloc)(alloc, capacity, sizeof(SLOT));
    ASSERT(slots);

    for (INDEX_TYPE index = 0; index < capacity; index++) {
        NS(SLOT, memory_tracker_empty)(&slots[index]);
    }

    return (SELF){
        .slots = slots,
        .capacity = (INDEX_TYPE)capacity,
        .free_list = INDEX_NONE,
        .exclusive_end = 0,
        .count = 0,
        .alloc = alloc,
        .derive_c_arena_basic = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static INDEX NS(SELF, insert)(SELF* self, VALUE value) {
    INVARIANT_CHECK(self);
    ASSERT(self->count < MAX_INDEX);

    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    if (self->free_list != INDEX_NONE) {
        INDEX_TYPE free_index = self->free_list;
        SLOT* slot = &self->slots[free_index];
        ASSUME(!slot->present);
        self->free_list = slot->next_free;
        slot->present = true;
        NS(SLOT, memory_tracker_present)(slot);
        slot->value = value;
        self->count++;
        return (INDEX){.index = free_index};
    }

    if (self->exclusive_end == self->capacity) {
        ASSERT(self->capacity <= (CAPACITY_EXCLUSIVE_UPPER / RESIZE_FACTOR));
        self->capacity *= RESIZE_FACTOR;
        SLOT* new_alloc = (SLOT*)realloc(self->slots, self->capacity * sizeof(SLOT));
        ASSERT(new_alloc);
        self->slots = new_alloc;

        for (size_t index = self->exclusive_end; index < self->capacity; index++) {
            NS(SLOT, memory_tracker_empty)(&self->slots[index]);
        }
    }

    INDEX_TYPE new_index = self->exclusive_end;
    SLOT* slot = &self->slots[new_index];
    slot->present = true;
    NS(SLOT, memory_tracker_present)(slot);
    slot->value = value;
    self->count++;
    self->exclusive_end++;
    return (INDEX){.index = new_index};
}

static VALUE* NS(SELF, try_write)(SELF* self, INDEX index) {
    INVARIANT_CHECK(self);
    if (!CHECK_ACCESS_INDEX(self, index)) {
        return NULL;
    }
    SLOT* slot = &self->slots[index.index];
    if (!slot->present) {
        return NULL;
    }
    return &slot->value;
}

static VALUE* NS(SELF, write)(SELF* self, INDEX index) {
    VALUE* value = NS(SELF, try_write)(self, index);
    ASSERT(value);
    return value;
}

static VALUE const* NS(SELF, try_read)(SELF const* self, INDEX index) {
    INVARIANT_CHECK(self);
    if (!CHECK_ACCESS_INDEX(self, index)) {
        return NULL;
    }
    SLOT* slot = &self->slots[index.index];
    if (!slot->present) {
        return NULL;
    }
    return &slot->value;
}

static VALUE const* NS(SELF, read)(SELF const* self, INDEX index) {
    VALUE const* value = NS(SELF, try_read)(self, index);
    ASSERT(value);
    return value;
}

static SELF NS(SELF, clone)(SELF const* self) {
    INVARIANT_CHECK(self);
    SLOT* slots = (SLOT*)NS(ALLOC, calloc)(self->alloc, self->capacity, sizeof(SLOT));
    ASSERT(slots);

    for (INDEX_TYPE index = 0; index < self->exclusive_end; index++) {
        if (self->slots[index].present) {
            NS(SLOT, memory_tracker_present)(&slots[index]);
            slots[index].present = true;
            slots[index].value = VALUE_CLONE(&self->slots[index].value);
        } else {
            NS(SLOT, memory_tracker_empty)(&slots[index]);
            slots[index].present = false;
            slots[index].next_free = self->slots[index].next_free;
        }
    }

    return (SELF){
        .slots = slots,
        .capacity = self->capacity,
        .free_list = self->free_list,
        .exclusive_end = self->exclusive_end,
        .count = self->count,
        .alloc = self->alloc,
        .derive_c_arena_basic = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static INDEX_TYPE NS(SELF, size)(SELF const* self) {
    INVARIANT_CHECK(self);
    return self->count;
}

static bool NS(SELF, full)(SELF const* self) {
    INVARIANT_CHECK(self);
    if (self->capacity == CAPACITY_EXCLUSIVE_UPPER) {
        if (self->free_list == INDEX_NONE) {
            return true;
        }
    }
    return false;
}

static const size_t NS(SELF, max_entries) = MAX_INDEX;

static bool NS(SELF, try_remove)(SELF* self, INDEX index, VALUE* destination) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    if (!CHECK_ACCESS_INDEX(self, index)) {
        return false;
    }

    SLOT* entry = &self->slots[index.index];
    if (entry->present) {
        *destination = entry->value;
        entry->present = false;
        NS(SLOT, memory_tracker_empty)(entry);
        entry->next_free = self->free_list;
        self->free_list = index.index;
        self->count--;
        return true;
    }
    return false;
}

static VALUE NS(SELF, remove)(SELF* self, INDEX index) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    VALUE value;
    ASSERT(NS(SELF, try_remove)(self, index, &value));
    return value;
}

static bool NS(SELF, delete_entry)(SELF* self, INDEX index) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    if (!CHECK_ACCESS_INDEX(self, index)) {
        return false;
    }

    SLOT* entry = &self->slots[index.index];
    if (entry->present) {
        VALUE_DELETE(&entry->value);
        entry->present = false;
        NS(SLOT, memory_tracker_empty)(entry);
        entry->next_free = self->free_list;
        self->free_list = index.index;
        self->count--;
        return true;
    }
    return false;
}

#define IV_PAIR NS(SELF, iv)
typedef struct {
    INDEX index;
    VALUE* value;
} IV_PAIR;

#define ITER NS(SELF, iter)
typedef IV_PAIR const* NS(ITER, item);

static bool NS(ITER, empty_item)(IV_PAIR const* const* item) { return *item == NULL; }

typedef struct {
    SELF* arena;
    INDEX_TYPE next_index;
    IV_PAIR curr;
    mutation_version version;
} ITER;

static bool NS(ITER, empty)(ITER const* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);
    // JUSTIFY: If no entries are left, then the previous '.._next' call moved
    //          the index to the exclusive end
    // NOTE: `index + 1 > exclusive_end` implies `index >= exclusive_end`
    return iter->next_index == INDEX_NONE || iter->next_index >= iter->arena->exclusive_end;
}

static IV_PAIR const* NS(ITER, next)(ITER* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);

    if (NS(ITER, empty)(iter)) {
        return NULL;
    }
    iter->curr = (IV_PAIR){.index = (INDEX){.index = iter->next_index},
                           .value = &iter->arena->slots[iter->next_index].value};
    iter->next_index++;
    while (iter->next_index < INDEX_NONE && iter->next_index < iter->arena->exclusive_end &&
           !iter->arena->slots[iter->next_index].present) {
        iter->next_index++;
    }

    return &iter->curr;
}

static ITER NS(SELF, get_iter)(SELF* self) {
    INVARIANT_CHECK(self);
    INDEX_TYPE index = 0;
    while (index < INDEX_NONE && index < self->exclusive_end && !self->slots[index].present) {
        index++;
    }

    return (ITER){
        .arena = self,
        .next_index = index,
        .curr = (IV_PAIR){.index = (INDEX){.index = INDEX_NONE}, .value = NULL},
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

static void NS(SELF, delete)(SELF* self) {
    INVARIANT_CHECK(self);
    ITER iter = NS(SELF, get_iter)(self);
    IV_PAIR const* entry;
    while ((entry = NS(ITER, next)(&iter))) {
        VALUE_DELETE(entry->value);
    }

    NS(ALLOC, free)(self->alloc, self->slots);
}

#undef ITER
#undef IV_PAIR

#define IV_PAIR_CONST NS(SELF, iv_const)
typedef struct {
    INDEX index;
    VALUE const* value;
} IV_PAIR_CONST;

#define ITER_CONST NS(SELF, iter_const)
typedef IV_PAIR_CONST const* NS(ITER_CONST, item);

static bool NS(ITER_CONST, empty_item)(IV_PAIR_CONST const* const* item) { return *item == NULL; }

typedef struct {
    SELF const* arena;
    INDEX_TYPE next_index;
    IV_PAIR_CONST curr;
    mutation_version version;
} ITER_CONST;

static bool NS(ITER_CONST, empty)(ITER_CONST const* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);
    return iter->next_index == INDEX_NONE || iter->next_index >= iter->arena->exclusive_end;
}

static IV_PAIR_CONST const* NS(ITER_CONST, next)(ITER_CONST* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);

    if (NS(ITER_CONST, empty)(iter)) {
        return NULL;
    }

    iter->curr = (IV_PAIR_CONST){.index = (INDEX){.index = iter->next_index},
                                 .value = &iter->arena->slots[iter->next_index].value};
    iter->next_index++;
    while (iter->next_index != INDEX_NONE && iter->next_index < iter->arena->exclusive_end &&
           !iter->arena->slots[iter->next_index].present) {
        iter->next_index++;
    }

    return &iter->curr;
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    INVARIANT_CHECK(self);
    INDEX_TYPE index = 0;
    while (index < INDEX_NONE && index < self->exclusive_end && !self->slots[index].present) {
        index++;
    }

    return (ITER_CONST){
        .arena = self,
        .next_index = index,
        .curr = (IV_PAIR_CONST){.index = (INDEX){.index = INDEX_NONE}, .value = NULL},
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

static void NS(SELF, debug)(SELF const* self, debug_fmt fmt, FILE* stream) {
    fprintf(stream, EXPAND_STRING(SELF) "@%p {\n", self);
    fmt = debug_fmt_scope_begin(fmt);
    debug_fmt_print(fmt, stream, "capacity: %lu,\n", self->capacity);
    debug_fmt_print(fmt, stream, "count: %lu,\n", self->count);
    debug_fmt_print(fmt, stream, "slots: %p,\n", self->slots);

    debug_fmt_print(fmt, stream, "alloc: ");
    NS(ALLOC, debug)(self->alloc, fmt, stream);
    fprintf(stream, ",\n");

    debug_fmt_print(fmt, stream, "items: [");
    fmt = debug_fmt_scope_begin(fmt);

    ITER_CONST iter = NS(SELF, get_iter_const)(self);
    IV_PAIR_CONST const* item;

    while ((item = NS(ITER_CONST, next)(&iter))) {
        debug_fmt_print(fmt, stream, "{\n");
        fmt = debug_fmt_scope_begin(fmt);

        debug_fmt_print(fmt, stream, "key: ");
        NS(INDEX, debug)(&item->index, fmt, stream);
        fprintf(stream, ",\n");

        debug_fmt_print(fmt, stream, "value: ");
        VALUE_DEBUG(item->value, fmt, stream);
        fprintf(stream, ",\n");

        fmt = debug_fmt_scope_end(fmt);
        debug_fmt_print(fmt, stream, "},\n");
    }

    fmt = debug_fmt_scope_end(fmt);
    debug_fmt_print(fmt, stream, "],\n");
    fmt = debug_fmt_scope_end(fmt);
    debug_fmt_print(fmt, stream, "}");
}

#undef ITER_CONST
#undef IV_PAIR_CONST

#undef VALUE
#undef VALUE_DELETE
#undef VALUE_CLONE
#undef VALUE_DEBUG

#include <derive-c/core/index/undef.h>

#undef SLOT
#undef CHECK_ACCESS_INDEX
#undef RESIZE_FACTOR
#undef INDEX

#undef INVARIANT_CHECK

#include <derive-c/core/alloc/undef.h>
TRAIT_ARENA(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/includes/undef.h>