/**
 * @file
 * @brief A vector-backed arena, with support for small indices. 
 */

#include <derive-c/core.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef PANIC
#error "PANIC must be defined (used for unrecoverable failures)"
#define PANIC abort() // Allows independent debugging
#endif

#ifndef INDEX_BITS
#error "The number of bits (8,16,32,64) needs to be specified"
#define INDEX_BITS 64
#endif

#if INDEX_BITS == 8
#define INDEX_TYPE uint8_t
#define MAX_CAPACITY (UINT8_MAX + 1)
#define MAX_INDEX (UINT8_MAX - 1)
#define INDEX_NONE UINT8_MAX
#elif INDEX_BITS == 16
#define INDEX_TYPE uint16_t
#define MAX_CAPACITY (UINT16_MAX + 1)
#define MAX_INDEX (UINT16_MAX - 1)
#define INDEX_NONE UINT16_MAX
#elif INDEX_BITS == 32
#define INDEX_TYPE uint32_t
#define MAX_CAPACITY (UINT32_MAX + 1)
#define MAX_INDEX (UINT32_MAX - 1)
#define INDEX_NONE UINT32_MAX
#elif INDEX_BITS == 64
#define INDEX_TYPE uint64_t
// NOTE: Special case, we cannot store the max capacity as a size_t integer
#define MAX_CAPACITY UINT64_MAX
#define MAX_INDEX (UINT64_MAX - 1)
#define INDEX_NONE UINT64_MAX
#else
#error "INDEX_BITS must be 8, 16, 32 or 64"
#endif

#ifndef V
#error "V (value type) must be defined for an arena template"
typedef struct {
    int x;
} placeholder_value;
#define V placeholder_value // Allows independent debugging
#endif

#ifndef SELF
#ifndef MODULE
#error                                                                                             \
    "MODULE must be defined to use a template (it is prepended to the start of all methods, and the type)"
#endif
#define SELF NAME(MODULE, NAME(arena, NAME(I, V)))
#endif

#define SLOT NAME(SELF, SLOT)

#define CHECK_ACCESS_INDEX(self, index)                                                            \
    DEBUG_ASSERT(self);                                                                            \
    ASSERT(index.index < self->exclusive_end);

// JUSTIFY: Macro rather than static
//           - Avoids the need to cast to the INDEX_TYPE
#define RESIZE_FACTOR 2

// INV: < MAX_CAPACITY
#define INDEX NAME(SELF, index)

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

/// @brief A vector-backed arena, with support for small indices.
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
} SELF;

static SELF NAME(SELF, new_with_capacity_for)(INDEX_TYPE items) {
    DEBUG_ASSERT(items > 0);
    size_t capacity = next_power_of_2(items);
    ASSERT(capacity <= MAX_CAPACITY);
    SLOT* slots = (SLOT*)calloc(capacity, sizeof(SLOT));
    if (!slots)
        PANIC;
    return (SELF){
        .slots = slots,
        .capacity = (INDEX_TYPE)capacity,
        .free_list = INDEX_NONE,
        .exclusive_end = 0,
        .count = 0,
    };
}

static INDEX NAME(SELF, insert)(SELF* self, V value) {
    DEBUG_ASSERT(self);
    if (self->free_list != INDEX_NONE) {
        INDEX_TYPE free_index = self->free_list;
        SLOT* slot = &self->slots[free_index];
        DEBUG_ASSERT(!SLOT->present);
        self->free_list = slot->next_free;
        slot->present = true;
        slot->value = value;
        self->count++;
        return (INDEX){.index = free_index};
    }

    if (self->exclusive_end == self->capacity) {
        // TODO: Better way to check this
        //        - maybe just have capacity as a size_t?
        ASSERT(self->capacity <= (MAX_CAPACITY / RESIZE_FACTOR));

        self->capacity *= RESIZE_FACTOR;
        SLOT* new_alloc = (SLOT*)realloc(self->slots, self->capacity * sizeof(SLOT));
        if (!new_alloc)
            PANIC;
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

static V* NAME(SELF, write)(SELF* self, INDEX index) {
    CHECK_ACCESS_INDEX(self, index);
    SLOT* slot = &self->slots[index.index];
    if (!slot->present) {
        return NULL;
    }
    return &slot->value;
}

static V* NAME(SELF, write_unchecked)(SELF* self, INDEX index) {
    DEBUG_ASSERT(self);
#ifdef NDEBUG
    V* value = NAME(SELF, write)(self, index);
    DEBUG_ASSERT(value);
    return value;
#endif
    return &self->slots[index.index].value;
}

static V const* NAME(SELF, read)(SELF const* self, INDEX index) {
    CHECK_ACCESS_INDEX(self, index);
    SLOT* slot = &self->slots[index.index];
    if (!slot->present) {
        return NULL;
    }
    return &slot->value;
}

static V const* NAME(SELF, read_unchecked)(SELF const* self, INDEX index) {
    DEBUG_ASSERT(self);
#ifdef NDEBUG
    V* value = NAME(SELF, read)(self, index);
    DEBUG_ASSERT(value);
    return value;
#endif
    return &self->slots[index.index].value;
}

static SELF NAME(SELF, clone)(SELF const* self) {
    DEBUG_ASSERT(self);
    SLOT* slots = (SLOT*)malloc(self->capacity * sizeof(SLOT));
    if (!slots)
        PANIC;
    memcpy(slots, self->slots, self->exclusive_end * sizeof(SLOT));
    return (SELF){
        .slots = slots,
        .capacity = self->capacity,
        .free_list = self->free_list,
        .exclusive_end = self->exclusive_end,
        .count = self->count,
    };
}

static INDEX_TYPE NAME(SELF, size)(SELF const* self) {
    DEBUG_ASSERT(self);
    return self->count;
}

static bool NAME(SELF, full)(SELF const* self) {
    DEBUG_ASSERT(self);
    if (self->capacity == MAX_CAPACITY) {
        if (self->free_list == INDEX_NONE) {
            return true;
        }
    }
    return false;
}

static size_t NAME(SELF, max_capacity) = MAX_CAPACITY;
static size_t NAME(SELF, max_index) = MAX_INDEX;

static void NAME(SELF, delete)(SELF* self) {
    DEBUG_ASSERT(self);
    free(self->slots);
}

#define REMOVED_ENTRY NAME(SELF, removed_entry)

typedef struct {
    union {
        V value;
    };
    bool present;
} REMOVED_ENTRY;

static REMOVED_ENTRY NAME(SELF, remove)(SELF* self, INDEX index) {
    CHECK_ACCESS_INDEX(self, index);
    SLOT* entry = &self->slots[index.index];
    if (entry->present) {
        REMOVED_ENTRY const ret_val = {.value = entry->value, .present = true};
        entry->present = false;
        entry->next_free = self->free_list;
        self->free_list = index.index;
        self->count--;
        return ret_val;
    }
    return (REMOVED_ENTRY){.present = false};
}

#undef REMOVED_ENTRY

#define IV_PAIR NAME(SELF, iv)
typedef struct {
    INDEX index;
    V* value;
} IV_PAIR;

#define ITER NAME(SELF, iter)
typedef struct {
    SELF* arena;
    INDEX_TYPE next_index;
    size_t pos;
} ITER;

static bool NAME(ITER, empty)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    // JUSTIFY: If no entries are left, then the previous '.._next' call moved
    //          the index to the exclusive end
    // NOTE: `index + 1 > exclusive_end` implies `index >= exclusive_end`
    return iter->next_index == INDEX_NONE || iter->next_index >= iter->arena->exclusive_end;
}

static IV_PAIR NAME(ITER, next)(ITER* iter) {
    DEBUG_ASSERT(iter);

    if (NAME(ITER, empty)(iter)) {
        return (IV_PAIR){.index = (INDEX){.index = INDEX_NONE}, .value = NULL};
    } else {
        IV_PAIR out = (IV_PAIR){.index = (INDEX){.index = iter->next_index},
                                .value = &iter->arena->slots[iter->next_index].value};
        iter->next_index++;
        while (iter->next_index < INDEX_NONE && iter->next_index < iter->arena->exclusive_end &&
               !iter->arena->slots[iter->next_index].present) {
            iter->next_index++;
        }

        iter->pos++;
        return out;
    }
}

static size_t NAME(ITER, position)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos;
}

static ITER NAME(SELF, get_iter)(SELF* self) {
    DEBUG_ASSERT(self);
    INDEX_TYPE index = 0;
    while (index < INDEX_NONE && index < self->exclusive_end && !self->slots[index].present) {
        index++;
    }

    return (ITER){
        .arena = self,
        .next_index = index,
        .pos = 0,
    };
}

#undef ITER
#undef IV_PAIR

#define IV_PAIR_CONST NAME(SELF, iv_const)
typedef struct {
    INDEX index;
    V const* value;
} IV_PAIR_CONST;

#define ITER_CONST NAME(SELF, iter_const)
typedef struct {
    SELF const* arena;
    INDEX_TYPE next_index;
    size_t pos;
} ITER_CONST;

static bool NAME(ITER_CONST, empty)(ITER_CONST const* iter) {
    DEBUG_ASSERT(iter);
    return iter->next_index == INDEX_NONE || iter->next_index >= iter->arena->exclusive_end;
}

static IV_PAIR_CONST NAME(ITER_CONST, next)(ITER_CONST* iter) {
    DEBUG_ASSERT(iter);

    if (NAME(ITER_CONST, empty)(iter)) {
        return (IV_PAIR_CONST){.index = (INDEX){.index = INDEX_NONE}, .value = NULL};
    } else {
        IV_PAIR_CONST out = (IV_PAIR_CONST){.index = (INDEX){.index = iter->next_index},
                                            .value = &iter->arena->slots[iter->next_index].value};
        iter->next_index++;
        while (iter->next_index != INDEX_NONE && iter->next_index < iter->arena->exclusive_end &&
               !iter->arena->slots[iter->next_index].present) {
            iter->next_index++;
        }

        iter->pos++;
        return out;
    }
}

static size_t NAME(ITER_CONST, position)(ITER_CONST const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos;
}

static ITER_CONST NAME(SELF, get_iter_const)(SELF const* self) {
    DEBUG_ASSERT(self);
    INDEX_TYPE index = 0;
    while (index < INDEX_NONE && index < self->exclusive_end && !self->slots[index].present) {
        index++;
    }

    return (ITER_CONST){
        .arena = self,
        .next_index = index,
        .pos = 0,
    };
}

#undef ITER_CONST
#undef IV_PAIR_CONST

#undef INDEX_BITS
#undef INDEX_TYPE
#undef MAX_CAPACITY
#undef MAX_INDEX
#undef INDEX_NONE
#undef SLOT
#undef CHECK_ACCESS_INDEX
#undef RESIZE_FACTOR
#undef INDEX

#undef V
#undef SELF
