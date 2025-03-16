#include <derive-c/core.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#ifndef PANIC
#error "PANIC must be defined (used for unrecoverable failures)"
#define PANIC abort() // Allows independent debugging
#endif

#ifndef I
#error "I (index type) must be defined for an arena template, chose a value like uint8_t, uint16_t, uint32_t"
#define I uint64_t
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

#define CHECK_ACCESS_INDEX(self, index) \
    DEBUG_ASSERT(self); \
    ASSERT(index < self->first_uninit_entry); \
    ASSERT(index > ZERO_INDEX); \

// JUSTIFY: Indexes start from 1
//           - In the free list we need to represent an `Option<Index>`, rather 
//             than storing a boolean (present) as well as the value, we just 
//             ensure indexes are never 0.
// JUSTIFY: Macro rather than static
//           - Avoids the need to cast to the index type
#define ZERO_INDEX 0 
#define INDEX_START 1
#define RESIZE_FACTOR 2

typedef struct {
    union {
        V value;
        I next_free;
    };

    // JUSTIFY: Present flag last
    //           - Reduces size, C ABI orders fields, placing it before a larger 
    //             (aligned) value would add (alignment - 1 byte) of unecessary 
    //             padding
    bool present;
} SLOT;

typedef struct {
    SLOT* slots;
    I capacity;
    I free_list;

    // JUSTIFY: Separately recording the first_uninit_entry
    //           - Allows us to not use calloc for the buffer (for < first_uninit_entry 
    //             we can check represent, for >= first_uninit_entry we know none are 
    //             present so having uninitialised entries is fine)
    I first_uninit_entry;

    // INVARIANT: If free_list == ZERO_INDEX, then all values from [0, count) 
    //            are present
    I count;
} SELF;

SELF NAME(SELF, new_with_capacity)(I capacity) {
    DEBUG_ASSERT(capacity > 0);
    SLOT* slots = (SLOT*)calloc(capacity, sizeof(SLOT));
    if (!slots) PANIC;
    return (SELF){
        .slots = slots,
        .capacity = capacity,
        .free_list = ZERO_INDEX,
        .first_uninit_entry = ZERO_INDEX,
        .count = 0,
    };
}

I NAME(SELF, insert)(SELF* self, V value) {
    DEBUG_ASSERT(self);
    if (self->free_list != ZERO_INDEX) {
        I reused_index = self->free_list;
        SLOT* slot = &self->slots[reused_index - INDEX_START];
        DEBUG_ASSERT(!SLOT->present);
        self->free_list = slot->next_free;
        slot->present = true;
        slot->value = value;
        self->count++;
        return reused_index;
    }
    if (self->first_uninit_entry == self->capacity) {
        self->capacity *= RESIZE_FACTOR;
        SLOT* new_alloc = realloc(self->slots, self->capacity);
        if (!new_alloc) PANIC;
        self->slots = new_alloc;
    }

    I new_index = self->first_uninit_entry; // index = (size - 1) + INDEX_START
    SLOT* slot = &self->slots[new_index - INDEX_START];
    slot->present = true;
    slot->value = value;
    self->count++;
    self->first_uninit_entry++;
    return new_index;
}

MAYBE_NULL(V) NAME(SELF, write)(SELF* self, I index) {
    CHECK_ACCESS_INDEX(self, index);
    SLOT* slot = &self->slots[index - INDEX_START];
    if (!slot->present) {
        return NULL;
    }
    return &slot->value;
}

NEVER_NULL(V) NAME(SELF, write_unchecked)(SELF* self, I index) {
    DEBUG_ASSERT(self);
#ifdef NDEBUG
    V* value = NAME(SELF, write)(self, index);
    DEBUG_ASSERT(value);
    return value;
#endif
    return &self->slots[index - INDEX_START].value;
}

MAYBE_NULL(V const) NAME(SELF, read)(SELF const* self, I index) {
    CHECK_ACCESS_INDEX(self, index);
    SLOT* slot = &self->slots[index - INDEX_START];
    if (!slot->present) {
        return NULL;
    }
    return &slot->value;
}

NEVER_NULL(V const) NAME(SELF, read_unchecked)(SELF const* self, I index) {
    DEBUG_ASSERT(self);
#ifdef NDEBUG
    V* value = NAME(SELF, read)(self, index);
    DEBUG_ASSERT(value);
    return value;
#endif
    return &self->slots[index - INDEX_START].value;
}

bool NAME(SELF, remove)(SELF* self, I index) {
    CHECK_ACCESS_INDEX(self, index);
    SLOT* entry = &self->slots[index - INDEX_START];
    if (entry->present) {
        entry->present = false;
        entry->next_free = self->free_list;
        self->free_list = index;
        self->count--;
        return true;
    }
    return false;
}

#define IV_PAIR NAME(SELF, iv)
typedef struct {
    I index;
    V* value;
} IV_PAIR;

#define ITER NAME(SELF, iter)

typedef struct {
    SELF* arena;
    I next_index;
    size_t pos;
} ITER;

IV_PAIR NAME(ITER, next)(ITER* iter) {
    DEBUG_ASSERT(iter);
    if (iter->next_index >= iter->arena->first_uninit_entry) {
        return (IV_PAIR){
            .index = ZERO_INDEX,
            .value = NULL
        };
    } else {
        IV_PAIR out = (IV_PAIR){
            .index = iter->next_index,
            .value = &iter->arena->slots[iter->next_index].value
        };
        while (iter->next_index < iter->arena->first_uninit_entry && !iter->arena->slots[iter->next_index - INDEX_START].present) {
            iter->next_index++;
        }
        iter->pos++;
        return out;
    }
}

size_t NAME(ITER, position)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    return iter->pos;
}

bool NAME(ITER, empty)(ITER const* iter) {
    DEBUG_ASSERT(iter);
    // JUSTIFY: If no entries are left, then the previous '.._next' call moved 
    //          the index to the first uninit entry. 
    return iter->next_index < iter->arena->first_uninit_entry;
}

ITER NAME(SELF, get_iter)(SELF* self) {
    DEBUG_ASSERT(self);
    
    I index = ZERO_INDEX;
    while (index < self->first_uninit_entry && !self->slots[index].present) {
        index++;
    }

    return (IV_PAIR){
        .index = index
    }
}
