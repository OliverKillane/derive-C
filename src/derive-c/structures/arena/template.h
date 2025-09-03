/// @brief A vector-backed arena, with support for small indices.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined INDEX_BITS
    #if !defined __clang_analyzer__
        #error "The number of bits (8,16,32,64) to use for the arena's key"
    #endif
    #define INDEX_BITS 32
#endif

#if !defined V
    #if !defined __clang_analyzer__
        #error "The value type to place in the arena must be defined"
    #endif
typedef struct {
    int x;
} derive_c_parameter_value;
    #define V derive_c_parameter_value
void derive_c_parameter_value_delete(derive_c_parameter_value* UNUSED(key)) {}
    #define V_DELETE derive_c_parameter_value_delete
#endif

#if !defined V_DELETE
    #define V_DELETE(value) (void)value
#endif

#if INDEX_BITS == 8
    #define INDEX_TYPE uint8_t
    #define MAX_CAPACITY (UINT8_MAX + 1ULL)
    #define MAX_INDEX (UINT8_MAX - 1ULL)
    #define INDEX_NONE UINT8_MAX
#elif INDEX_BITS == 16
    #define INDEX_TYPE uint16_t
    #define MAX_CAPACITY (UINT16_MAX + 1ULL)
    #define MAX_INDEX (UINT16_MAX - 1ULL)
    #define INDEX_NONE UINT16_MAX
#elif INDEX_BITS == 32
    #define INDEX_TYPE uint32_t
    #define MAX_CAPACITY (UINT32_MAX + 1ULL)
    #define MAX_INDEX (UINT32_MAX - 1ULL)
    #define INDEX_NONE UINT32_MAX
#elif INDEX_BITS == 64
    #define INDEX_TYPE uint64_t
    // JUSTIFY: Special case, we cannot store the max capacity as a size_t integer
    #define MAX_CAPACITY UINT64_MAX
    #define MAX_INDEX (UINT64_MAX - 1ULL)
    #define INDEX_NONE UINT64_MAX
#endif

#define SLOT NS(SELF, SLOT)

#define CHECK_ACCESS_INDEX(self, index) ((index).index < (self)->exclusive_end)

// JUSTIFY: Macro rather than static
//           - Avoids the need to cast to the INDEX_TYPE
#define RESIZE_FACTOR 2

// INV: < MAX_CAPACITY
#define INDEX NS(SELF, index)

typedef struct {
    INDEX_TYPE index;
} INDEX;

typedef struct {
    union {
        V value;
        INDEX_TYPE next_free;
    };

    // JUSTIFY: Present flag last
    //           - Reduces size, C ABI orders fields, placing it before a larger
    //             (aligned) value would add (alignment - 1 byte) of unecessary
    //             padding
    bool present;
} SLOT;

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
} SELF;

static SELF NS(SELF, new_with_capacity_for)(INDEX_TYPE items, ALLOC* alloc) {
    DEBUG_ASSERT(items > 0);
    size_t capacity = next_power_of_2(items);
    ASSERT(capacity <= MAX_CAPACITY);
    SLOT* slots = (SLOT*)NS(ALLOC, calloc)(alloc, capacity, sizeof(SLOT));
    ASSERT(slots);
    return (SELF){
        .slots = slots,
        .capacity = (INDEX_TYPE)capacity,
        .free_list = INDEX_NONE,
        .exclusive_end = 0,
        .count = 0,
        .alloc = alloc,
    };
}

static INDEX NS(SELF, insert)(SELF* self, V value) {
    DEBUG_ASSERT(self);
    if (self->free_list != INDEX_NONE) {
        INDEX_TYPE free_index = self->free_list;
        SLOT* slot = &self->slots[free_index];
        DEBUG_ASSERT(!slot->present);
        self->free_list = slot->next_free;
        slot->present = true;
        slot->value = value;
        self->count++;
        return (INDEX){.index = free_index};
    }

    if (self->exclusive_end == self->capacity) {
        ASSERT(self->capacity <= (MAX_CAPACITY / RESIZE_FACTOR));
        self->capacity *= RESIZE_FACTOR;
        SLOT* new_alloc = (SLOT*)realloc(self->slots, self->capacity * sizeof(SLOT));
        ASSERT(new_alloc);
        self->slots = new_alloc;
    }

    INDEX_TYPE new_index = self->exclusive_end;
    SLOT* slot = &self->slots[new_index];
    slot->present = true;
    slot->value = value;
    self->count++;
    self->exclusive_end++;
    return (INDEX){.index = new_index};
}

static V* NS(SELF, try_write)(SELF* self, INDEX index) {
    DEBUG_ASSERT(self);
    if (!CHECK_ACCESS_INDEX(self, index)) {
        return NULL;
    }
    SLOT* slot = &self->slots[index.index];
    if (!slot->present) {
        return NULL;
    }
    return &slot->value;
}

static V* NS(SELF, write)(SELF* self, INDEX index) {
    V* value = NS(SELF, try_write)(self, index);
    ASSERT(value);
    return value;
}

static V const* NS(SELF, try_read)(SELF const* self, INDEX index) {
    DEBUG_ASSERT(self);
    if (!CHECK_ACCESS_INDEX(self, index)) {
        return NULL;
    }
    SLOT* slot = &self->slots[index.index];
    if (!slot->present) {
        return NULL;
    }
    return &slot->value;
}

static V const* NS(SELF, read)(SELF const* self, INDEX index) {
    V const* value = NS(SELF, try_read)(self, index);
    ASSERT(value);
    return value;
}

static SELF NS(SELF, shallow_clone)(SELF const* self) {
    DEBUG_ASSERT(self);
    SLOT* slots = (SLOT*)NS(ALLOC, calloc)(self->alloc, self->capacity, sizeof(SLOT));
    ASSERT(slots);
    memcpy(slots, self->slots, self->exclusive_end * sizeof(SLOT));
    return (SELF){
        .slots = slots,
        .capacity = self->capacity,
        .free_list = self->free_list,
        .exclusive_end = self->exclusive_end,
        .count = self->count,
        .alloc = self->alloc,
    };
}

static INDEX_TYPE NS(SELF, size)(SELF const* self) {
    DEBUG_ASSERT(self);
    return self->count;
}

static bool NS(SELF, full)(SELF const* self) {
    DEBUG_ASSERT(self);
    if (self->capacity == MAX_CAPACITY) {
        if (self->free_list == INDEX_NONE) {
            return true;
        }
    }
    return false;
}

static size_t NS(SELF, max_capacity) = MAX_CAPACITY;
static size_t NS(SELF, max_index) = MAX_INDEX;

static bool NS(SELF, try_remove)(SELF* self, INDEX index, V* destination) {
    DEBUG_ASSERT(self);
    if (!CHECK_ACCESS_INDEX(self, index)) {
        return false;
    }

    SLOT* entry = &self->slots[index.index];
    if (entry->present) {
        *destination = entry->value;
        entry->present = false;
        entry->next_free = self->free_list;
        self->free_list = index.index;
        self->count--;
        return true;
    }
    return false;
}

static V NS(SELF, remove)(SELF* self, INDEX index) {
    V value;
    ASSERT(NS(SELF, try_remove)(self, index, &value));
    return value;
}

static bool NS(SELF, delete_entry)(SELF* self, INDEX index) {
    DEBUG_ASSERT(self);
    if (!CHECK_ACCESS_INDEX(self, index)) {
        return false;
    }

    SLOT* entry = &self->slots[index.index];
    if (entry->present) {
        V_DELETE(&entry->value);
        entry->present = false;
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
    V* value;
} IV_PAIR;

#define ITER NS(SELF, iter)
typedef struct {
    SELF* arena;
    INDEX_TYPE next_index;
    size_t pos;
    IV_PAIR curr;
} ITER;

static bool NS(ITER, empty)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    // JUSTIFY: If no entries are left, then the previous '.._next' call moved
    //          the index to the exclusive end
    // NOTE: `index + 1 > exclusive_end` implies `index >= exclusive_end`
    return iter->next_index == INDEX_NONE || iter->next_index >= iter->arena->exclusive_end;
}

static IV_PAIR const* NS(ITER, next)(ITER* iter) {
    DEBUG_ASSERT(iter);

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

    iter->pos++;
    return &iter->curr;
}

static size_t NS(ITER, position)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos;
}

static ITER NS(SELF, get_iter)(SELF* self) {
    DEBUG_ASSERT(self);
    INDEX_TYPE index = 0;
    while (index < INDEX_NONE && index < self->exclusive_end && !self->slots[index].present) {
        index++;
    }

    return (ITER){
        .arena = self,
        .next_index = index,
        .pos = 0,
        .curr = (IV_PAIR){.index = (INDEX){.index = INDEX_NONE}, .value = NULL},
    };
}

static void NS(SELF, delete)(SELF* self) {
    DEBUG_ASSERT(self);
    ITER iter = NS(SELF, get_iter)(self);
    IV_PAIR const* entry;
    while ((entry = NS(ITER, next)(&iter))) {
        V_DELETE(entry->value);
    }

    NS(ALLOC, free)(self->alloc, self->slots);
}

#undef ITER
#undef IV_PAIR

#define IV_PAIR_CONST NS(SELF, iv_const)
typedef struct {
    INDEX index;
    V const* value;
} IV_PAIR_CONST;

static IV_PAIR_CONST NS(SELF, iv_const_empty) = {
    .index = {.index = INDEX_NONE},
    .value = NULL,
};

#define ITER_CONST NS(SELF, iter_const)
typedef struct {
    SELF const* arena;
    INDEX_TYPE next_index;
    size_t pos;
    IV_PAIR_CONST curr;
} ITER_CONST;

static bool NS(ITER_CONST, empty)(ITER_CONST const* iter) {
    DEBUG_ASSERT(iter);
    return iter->next_index == INDEX_NONE || iter->next_index >= iter->arena->exclusive_end;
}

static IV_PAIR_CONST const* NS(ITER_CONST, next)(ITER_CONST* iter) {
    DEBUG_ASSERT(iter);

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

    iter->pos++;
    return &iter->curr;
}

static size_t NS(ITER_CONST, position)(ITER_CONST const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos;
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    DEBUG_ASSERT(self);
    INDEX_TYPE index = 0;
    while (index < INDEX_NONE && index < self->exclusive_end && !self->slots[index].present) {
        index++;
    }

    return (ITER_CONST){
        .arena = self,
        .next_index = index,
        .pos = 0,
        .curr = (IV_PAIR_CONST){.index = (INDEX){.index = INDEX_NONE}, .value = NULL},
    };
}

#undef ITER_CONST
#undef IV_PAIR_CONST

#undef ALLOC
#undef INDEX_BITS
#undef V
#undef V_DELETE

#undef INDEX_TYPE
#undef MAX_CAPACITY
#undef MAX_INDEX
#undef INDEX_NONE

#undef SLOT
#undef CHECK_ACCESS_INDEX
#undef RESIZE_FACTOR
#undef INDEX

#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/self/undef.h>
