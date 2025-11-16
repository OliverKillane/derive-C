/// @brief A block based arena that allocated in block doubling in size.
/// TODO(oliverkillane): look at bumaplo
///
/// Blocks of doubling size are used to allocate elements.
///  - Block determined by checking the most significant bit
///  - Allows for efficient lookup of the block index
/// For example with:
/// ```c
/// INITIAL_BLOCK_INDEX_BITS = 3
/// INDEX_BITS = 8
/// ```
///
/// | BitMask  | Index | Entries | Offset | Block |
/// |----------|-------|---------|--------|-------|
/// | XXXXX000 |     0 |       8 |      0 |     0 |
/// | XXXXX111 |     7 |         |      7 |     0 |
/// | XXXX1000 |     8 |       8 |      0 |     1 |
/// | XXXX1111 |    15 |         |      7 |     1 |
/// | XXX10000 |    16 |      16 |      0 |     2 |
/// | XXX11111 |    31 |         |     15 |     2 |
/// | XX100000 |    32 |      32 |      0 |     3 |
/// | XX111111 |    63 |         |     31 |     3 |
/// | X1000000 |    64 |      64 |      0 |     4 |
/// | X1111111 |   127 |         |     63 |     4 |
/// | 10000000 |   128 |     128 |      0 |     5 |
/// | 11111111 |   255 |         |    127 |     5 |
///

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <derive-c/container/arena/trait.h>
#include <derive-c/core/debug/gdb_marker.h>
#include <derive-c/core/debug/memory_tracker.h>
#include <derive-c/core/debug/mutation_tracker.h>
#include <derive-c/core/prelude.h>

#include "utils.h"

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

#if !defined INITIAL_BLOCK_INDEX_BITS
    #define INITIAL_BLOCK_INDEX_BITS 8
#endif

STATIC_ASSERT(INITIAL_BLOCK_INDEX_BITS < INDEX_BITS,
              "INITIAL_BLOCK_INDEX_BITS must be less than INDEX_BITS");
STATIC_ASSERT(INITIAL_BLOCK_INDEX_BITS > 0, "INITIAL_BLOCK_INDEX_BITS must be greater than zero");

static const size_t NS(SELF, max_entries) = MAX_INDEX;

#define INDEX NS(SELF, index_t)

typedef VALUE NS(SELF, value_t);
typedef ALLOC NS(SELF, alloc_t);

typedef struct {
    INDEX_TYPE index;
} INDEX;

#define SLOT NS(SELF, slot)

typedef struct {
    union {
        VALUE value;
        INDEX_TYPE next_free;
    };
    bool present;
} SLOT;

static void PRIV(NS(SLOT, memory_tracker_empty))(SLOT const* slot) {
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE, &slot->value,
                       sizeof(VALUE));
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_WRITE, &slot->next_free,
                       sizeof(INDEX_TYPE));
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_READ_WRITE, &slot->present,
                       sizeof(bool));
}

static void PRIV(NS(SLOT, set_empty))(SLOT* slot, INDEX_TYPE next_free) {
    PRIV(NS(SLOT, memory_tracker_empty))(slot);
    slot->present = false;
    slot->next_free = next_free;
}

static void PRIV(NS(SLOT, memory_tracker_full))(SLOT const* slot) {
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE, &slot->next_free,
                       sizeof(INDEX_TYPE));
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_WRITE, &slot->value,
                       sizeof(VALUE));
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_READ_WRITE, &slot->present,
                       sizeof(bool));
}

static void PRIV(NS(SLOT, fill))(SLOT* slot, VALUE value) {
    PRIV(NS(SLOT, memory_tracker_full))(slot);
    slot->present = true;
    slot->value = value;
}

typedef struct {
    // INVARIANT: If free_list == EMPTY_INDEX, then all values from [0, count)
    //            are present
    INDEX_TYPE free_list;
    size_t count;

    ALLOC* alloc;
    gdb_marker derive_c_arena_blocks;
    mutation_tracker iterator_invalidation_tracker;

    // JUSTIFY: Using index type for the block exclusive end
    //  - We can use a smaller integer type (than size_t), as this is guarenteed to be smaller
    //    half the largest index.
    size_t block_current_exclusive_end;
    uint8_t block_current;
    SLOT* blocks[MAX_NUM_BLOCKS(INDEX_BITS, INITIAL_BLOCK_INDEX_BITS)];
} SELF;

static void PRIV(NS(SELF, set_memory_tracking))(SELF const* self) {
    for (uint8_t block_index = 0; block_index <= self->block_current; block_index++) {
        size_t block_items = block_index == self->block_current
                                 ? self->block_current_exclusive_end
                                 : BLOCK_TO_SIZE(block_index, INITIAL_BLOCK_INDEX_BITS);
        for (size_t offset = 0; offset < block_items; offset++) {
            SLOT* slot = &self->blocks[block_index][offset];
            if (slot->present) {
                PRIV(NS(SLOT, memory_tracker_full))(slot);
            } else {
                PRIV(NS(SLOT, memory_tracker_empty))(slot);
            }
        }
    }

    size_t tail_slots = BLOCK_TO_SIZE(self->block_current, INITIAL_BLOCK_INDEX_BITS) -
                        (self->block_current_exclusive_end - 1);
    memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_NONE,
                       &self->blocks[self->block_current][self->block_current_exclusive_end],
                       tail_slots * sizeof(SLOT));
}

#define INVARIANT_CHECK(self)                                                                      \
    ASSUME(self);                                                                                  \
    ASSUME(BLOCK_TO_SIZE((self)->block_current, INITIAL_BLOCK_INDEX_BITS) >=                       \
           (self)->block_current_exclusive_end);                                                   \
    ASSUME((self)->count <= MAX_INDEX);

static SELF NS(SELF, new)(ALLOC* alloc) {
    uint8_t initial_block = 0;
    size_t initial_block_items = BLOCK_TO_SIZE(initial_block, INITIAL_BLOCK_INDEX_BITS);
    SLOT* initial_block_slots = (SLOT*)NS(ALLOC, malloc)(alloc, initial_block_items * sizeof(SLOT));
    ASSERT(initial_block_slots);

    SELF self = {
        .free_list = INDEX_NONE,
        .count = 0,
        .alloc = alloc,
        .derive_c_arena_blocks = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
        .block_current_exclusive_end = 0,
        .block_current = initial_block,
        .blocks =
            {
                initial_block_slots,
            },
    };

    PRIV(NS(SELF, set_memory_tracking))(&self);
    return self;
}

static INDEX NS(SELF, insert)(SELF* self, VALUE value) {
    INVARIANT_CHECK(self);
    ASSERT(self->count < MAX_INDEX);

    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    if (self->free_list != INDEX_NONE) {
        INDEX_TYPE free_index = self->free_list;

        uint8_t block = INDEX_TO_BLOCK(free_index, INITIAL_BLOCK_INDEX_BITS);
        size_t offset = INDEX_TO_OFFSET(free_index, block, INITIAL_BLOCK_INDEX_BITS);
        SLOT* free_slot = &self->blocks[block][offset];

        ASSUME(!free_slot->present, "The free list should only contain free slots");
        self->free_list = free_slot->next_free;

        free_slot->present = true;
        PRIV(NS(SLOT, fill))(free_slot, value);
        self->count++;

        return (INDEX){.index = free_index};
    }

    if (self->block_current_exclusive_end ==
        BLOCK_TO_SIZE(self->block_current, INITIAL_BLOCK_INDEX_BITS)) {
        ASSUME(self->block_current < sizeof(self->blocks) / sizeof(SLOT*));

        self->block_current++;
        size_t block_items = BLOCK_TO_SIZE(self->block_current, INITIAL_BLOCK_INDEX_BITS);
        SLOT* block_slots = (SLOT*)NS(ALLOC, malloc)(self->alloc, block_items * sizeof(SLOT));
        ASSERT(block_slots);

        self->blocks[self->block_current] = block_slots;
        self->block_current++;
        self->block_current_exclusive_end = 0;
    }

    size_t offset = self->block_current_exclusive_end;
    PRIV(NS(SLOT, fill))(&self->blocks[self->block_current][offset], value);
    INDEX_TYPE new_index =
        (INDEX_TYPE)(BLOCK_OFFSET_TO_INDEX(self->block_current, offset, INITIAL_BLOCK_INDEX_BITS));

    self->block_current_exclusive_end++;
    self->count++;

    PRIV(NS(SELF, set_memory_tracking))(self);

    return (INDEX){.index = new_index};
}

static VALUE const* NS(SELF, try_read)(SELF const* self, INDEX index) {
    INVARIANT_CHECK(self);

    uint8_t block = INDEX_TO_BLOCK(index.index, INITIAL_BLOCK_INDEX_BITS);
    if (block > self->block_current) {
        return NULL;
    }

    size_t offset = INDEX_TO_OFFSET(index.index, block, INITIAL_BLOCK_INDEX_BITS);

    if (block == self->block_current && offset >= self->block_current_exclusive_end) {
        return NULL;
    }

    SLOT* slot = &self->blocks[block][offset];
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

static VALUE* NS(SELF, try_write)(SELF* self, INDEX index) {
    return (VALUE*)NS(SELF, try_read)(self, index);
}

static VALUE* NS(SELF, write)(SELF* self, INDEX index) {
    return (VALUE*)NS(SELF, read)(self, index);
}

static INDEX_TYPE NS(SELF, size)(SELF const* self) {
    INVARIANT_CHECK(self);
    return self->count;
}

static SELF NS(SELF, clone)(SELF const* self) {
    INVARIANT_CHECK(self);

    SELF new_self = {
        .free_list = self->free_list,
        .count = self->count,
        .alloc = self->alloc,
        .derive_c_arena_blocks = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
        .block_current_exclusive_end = self->block_current_exclusive_end,
        .block_current = self->block_current,
        .blocks = {},
    };

    for (size_t block_index = 0; block_index <= self->block_current; block_index++) {
        size_t block_items = BLOCK_TO_SIZE(block_index, INITIAL_BLOCK_INDEX_BITS);
        SLOT* block_slots = (SLOT*)NS(ALLOC, malloc)(self->alloc, block_items * sizeof(SLOT));
        ASSERT(block_slots);
        new_self.blocks[block_index] = block_slots;

        size_t const to_offset =
            block_index == self->block_current ? self->block_current_exclusive_end : block_items;

        for (size_t offset = 0; offset < to_offset; offset++) {
            SLOT* src_slot = &self->blocks[block_index][offset];
            SLOT* dst_slot = &new_self.blocks[block_index][offset];

            if (src_slot->present) {
                dst_slot->present = true;
                dst_slot->value = VALUE_CLONE(&src_slot->value);
                new_self.count++;
            } else {
                dst_slot->present = false;
            }
        }
    }

    PRIV(NS(SELF, set_memory_tracking))(&new_self);

    return new_self;
}

static bool NS(SELF, try_remove)(SELF* self, INDEX index, VALUE* destination) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    uint8_t block = INDEX_TO_BLOCK(index.index, INITIAL_BLOCK_INDEX_BITS);
    if (block > self->block_current) {
        return false;
    }

    size_t offset = INDEX_TO_OFFSET(index.index, block, INITIAL_BLOCK_INDEX_BITS);

    if (block == self->block_current && offset >= self->block_current_exclusive_end) {
        return false;
    }

    SLOT* slot = &self->blocks[block][offset];
    if (slot->present) {
        *destination = slot->value;
        slot->present = false;
        PRIV(NS(SLOT, set_empty))(slot, self->free_list);
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

static void NS(SELF, delete)(SELF* self) {
    INVARIANT_CHECK(self);

    for (uint8_t block = 0; block <= self->block_current; block++) {
        size_t const to_offset = block == self->block_current
                                     ? self->block_current_exclusive_end
                                     : BLOCK_TO_SIZE(block, INITIAL_BLOCK_INDEX_BITS);

        for (size_t offset = 0; offset < to_offset; offset++) {
            SLOT* slot = &self->blocks[block][offset];
            if (slot->present) {
                VALUE_DELETE(&slot->value);
            }
        }

        memory_tracker_set(MEMORY_TRACKER_LVL_CONTAINER, MEMORY_TRACKER_CAP_WRITE,
                           self->blocks[block], BLOCK_TO_SIZE(block, INITIAL_BLOCK_INDEX_BITS));
        NS(ALLOC, free)(self->alloc, self->blocks[block]);
    }
}

#define IV_PAIR_CONST NS(SELF, iv_const)
typedef struct {
    INDEX index;
    VALUE const* value;
} IV_PAIR_CONST;

static const IV_PAIR_CONST NS(SELF, iv_const_empty) = {
    .index = {.index = INDEX_NONE},
    .value = NULL,
};

#define ITER_CONST NS(SELF, iter_const)
typedef IV_PAIR_CONST NS(ITER_CONST, item);

static bool NS(ITER_CONST, empty_item)(IV_PAIR_CONST const* item) { return item->value == NULL; }

typedef struct {
    SELF const* arena;
    INDEX_TYPE next_index;
    mutation_version version;
} ITER_CONST;

static IV_PAIR_CONST NS(ITER_CONST, next)(ITER_CONST* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);

    while (iter->next_index < MAX_INDEX) {
        uint8_t block = INDEX_TO_BLOCK(iter->next_index, INITIAL_BLOCK_INDEX_BITS);
        size_t offset = INDEX_TO_OFFSET(iter->next_index, block, INITIAL_BLOCK_INDEX_BITS);

        if (block == iter->arena->block_current &&
            offset >= iter->arena->block_current_exclusive_end) {
            break;
        }

        SLOT* slot = &iter->arena->blocks[block][offset];
        if (slot->present) {
            IV_PAIR_CONST result = {
                .index = (INDEX){.index = iter->next_index},
                .value = &slot->value,
            };
            iter->next_index++;
            return result;
        }

        iter->next_index++;
    }

    return NS(SELF, iv_const_empty);
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    INVARIANT_CHECK(self);

    return (ITER_CONST){
        .arena = self,
        .next_index = 0,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

#undef IV_PAIR_CONST
#undef ITER_CONST

#define IV_PAIR NS(SELF, iv)
typedef struct {
    INDEX index;
    VALUE const* value;
} IV_PAIR;

static IV_PAIR NS(SELF, iv_empty) = {
    .index = {.index = INDEX_NONE},
    .value = NULL,
};

#define ITER NS(SELF, iter)
typedef IV_PAIR NS(ITER, item);

static bool NS(ITER, empty_item)(IV_PAIR const* item) { return item->value == NULL; }

typedef struct {
    SELF* arena;
    INDEX_TYPE next_index;
    mutation_version version;
} ITER;

static IV_PAIR NS(ITER, next)(ITER* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);

    while (iter->next_index < MAX_INDEX) {
        uint8_t block = INDEX_TO_BLOCK(iter->next_index, INITIAL_BLOCK_INDEX_BITS);
        size_t offset = INDEX_TO_OFFSET(iter->next_index, block, INITIAL_BLOCK_INDEX_BITS);

        if (block == iter->arena->block_current &&
            offset >= iter->arena->block_current_exclusive_end) {
            break;
        }

        SLOT* slot = &iter->arena->blocks[block][offset];
        if (slot->present) {
            IV_PAIR result = {
                .index = (INDEX){.index = iter->next_index},
                .value = &slot->value,
            };
            iter->next_index++;
            return result;
        }

        iter->next_index++;
    }

    return NS(SELF, iv_empty);
}

static ITER NS(SELF, get_iter)(SELF* self) {
    INVARIANT_CHECK(self);

    return (ITER){
        .arena = self,
        .next_index = 0,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

#undef IV_PAIR
#undef ITER

#include <derive-c/core/index/undef.h>
#undef INITIAL_BLOCK_INDEX_BITS

#undef VALUE
#undef VALUE_DELETE
#undef VALUE_CLONE
#undef VALUE_DEBUG

#undef SLOT
#undef INDEX

#include <derive-c/core/alloc/undef.h>
// TRAIT_ARENA(SELF);

#include <derive-c/core/self/undef.h>
