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

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#if !defined INDEX_BITS
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("The number of bits (8,16,32,64) to use for the arena's key")
    #endif
    #define INDEX_BITS 32
#endif

#if !defined VALUE
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("The value type to place in the arena must be defined")
    #endif
typedef struct {
    int x;
} value_t;
    #define VALUE value_t
    #define VALUE_DELETE value_delete
static void VALUE_DELETE(value_t* /* self */) {}
    #define VALUE_CLONE value_clone
static value_t VALUE_CLONE(value_t const* self) { return *self; }
    #define VALUE_DEBUG value_debug
static void VALUE_DEBUG(VALUE const* /* self */, dc_debug_fmt /* fmt */, FILE* /* stream */) {}
#endif

DC_STATIC_ASSERT(sizeof(VALUE), "VALUE must be a non-zero sized type");

#if !defined VALUE_DELETE
    #define VALUE_DELETE DC_NO_DELETE
#endif

#if !defined VALUE_CLONE
    #define VALUE_CLONE DC_COPY_CLONE
#endif

#if !defined VALUE_DEBUG
    #define VALUE_DEBUG DC_DEFAULT_DEBUG
#endif

#include <derive-c/core/index/bits_to_type/def.h>
#include <derive-c/core/index/type_to_strong/def.h>

#if !defined INITIAL_BLOCK_INDEX_BITS
    #define INITIAL_BLOCK_INDEX_BITS 8
#endif

DC_STATIC_ASSERT(INITIAL_BLOCK_INDEX_BITS < INDEX_BITS,
                 "INITIAL_BLOCK_INDEX_BITS must be less than INDEX_BITS");
DC_STATIC_ASSERT(INITIAL_BLOCK_INDEX_BITS > 0,
                 "INITIAL_BLOCK_INDEX_BITS must be greater than zero");

static const size_t NS(SELF, max_entries) = MAX_INDEX;

typedef VALUE NS(SELF, value_t);
typedef ALLOC NS(SELF, alloc_t);

#define SLOT NS(NAME, slot)

#define SLOT_INDEX_TYPE INDEX_TYPE     // [DERIVE-C] for template
#define SLOT_VALUE VALUE               // [DERIVE-C] for template
#define SLOT_VALUE_CLONE VALUE_CLONE   // [DERIVE-C] for template
#define SLOT_VALUE_DELETE VALUE_DELETE // [DERIVE-C] for template
#define INTERNAL_NAME SLOT             // [DERIVE-C] for template
#include <derive-c/utils/slot/template.h>

typedef struct {
    // INVARIANT: If free_list == EMPTY_INDEX, then all values from [0, count)
    //            are present
    INDEX_TYPE free_list;
    size_t count;

    NS(ALLOC, ref) alloc_ref;
    dc_gdb_marker derive_c_arena_blocks;
    mutation_tracker iterator_invalidation_tracker;

    size_t block_current_exclusive_end;
    // JUSTIFY: Block is a uint8_t
    //  - We can have at most 63 blocks, as we can have 64 bit indices, with an initial 1 bit size.
    uint8_t block_current;
    SLOT* blocks[DC_ARENA_GEO_MAX_NUM_BLOCKS(INDEX_BITS, INITIAL_BLOCK_INDEX_BITS)];
} SELF;

#define INVARIANT_CHECK(self)                                                                      \
    DC_ASSUME(self);                                                                               \
    DC_ASSUME(DC_ARENA_GEO_BLOCK_TO_SIZE((self)->block_current, INITIAL_BLOCK_INDEX_BITS) >=       \
              (self)->block_current_exclusive_end);                                                \
    DC_ASSUME((self)->count <= MAX_INDEX);

DC_PUBLIC static SELF NS(SELF, new)(NS(ALLOC, ref) alloc_ref) {
    uint8_t initial_block = 0;
    size_t initial_block_items =
        DC_ARENA_GEO_BLOCK_TO_SIZE(initial_block, INITIAL_BLOCK_INDEX_BITS);
    SLOT* initial_block_slots =
        (SLOT*)NS(ALLOC, allocate_uninit)(alloc_ref, initial_block_items * sizeof(SLOT));

    SELF self = {
        .free_list = INDEX_NONE,
        .count = 0,
        .alloc_ref = alloc_ref,
        .derive_c_arena_blocks = dc_gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
        .block_current_exclusive_end = 0,
        .block_current = initial_block,
        .blocks =
            {
                initial_block_slots,
            },
    };

    return self;
}

DC_PUBLIC static INDEX NS(SELF, insert)(SELF* self, VALUE value) {
    INVARIANT_CHECK(self);
    DC_ASSERT(self->count < MAX_INDEX,
              "Arena is full, cannot insert {count=%lu, max_index=%lu, value=%s}",
              (size_t)self->count, (size_t)MAX_INDEX, DC_DEBUG(VALUE_DEBUG, &value));

    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    if (self->free_list != INDEX_NONE) {
        INDEX_TYPE free_index = self->free_list;

        uint8_t block = DC_ARENA_GEO_INDEX_TO_BLOCK(free_index, INITIAL_BLOCK_INDEX_BITS);
        size_t offset = DC_ARENA_GEO_INDEX_TO_OFFSET(free_index, block, INITIAL_BLOCK_INDEX_BITS);
        SLOT* free_slot = &self->blocks[block][offset];

        DC_ASSUME(!free_slot->present, "The free list should only contain free slots");
        self->free_list = free_slot->next_free;

        NS(SLOT, fill)(free_slot, value);

        self->count++;

        return (INDEX){.index = free_index};
    }

    if (self->block_current_exclusive_end ==
        DC_ARENA_GEO_BLOCK_TO_SIZE(self->block_current, INITIAL_BLOCK_INDEX_BITS)) {
        DC_ASSUME(self->block_current < sizeof(self->blocks) / sizeof(SLOT*));

        self->block_current++;
        size_t block_items =
            DC_ARENA_GEO_BLOCK_TO_SIZE(self->block_current, INITIAL_BLOCK_INDEX_BITS);
        SLOT* block_slots =
            (SLOT*)NS(ALLOC, allocate_uninit)(self->alloc_ref, block_items * sizeof(SLOT));

        self->blocks[self->block_current] = block_slots;
        self->block_current_exclusive_end = 0;
    }

    size_t offset = self->block_current_exclusive_end;
    NS(SLOT, fill)(&self->blocks[self->block_current][offset], value);
    INDEX_TYPE new_index = (INDEX_TYPE)(DC_ARENA_GEO_BLOCK_OFFSET_TO_INDEX(
        self->block_current, offset, INITIAL_BLOCK_INDEX_BITS));

    self->block_current_exclusive_end++;
    self->count++;

    return (INDEX){.index = new_index};
}

DC_PUBLIC static VALUE const* NS(SELF, try_read)(SELF const* self, INDEX index) {
    INVARIANT_CHECK(self);

    uint8_t block = DC_ARENA_GEO_INDEX_TO_BLOCK(index.index, INITIAL_BLOCK_INDEX_BITS);
    if (block > self->block_current) {
        return NULL;
    }

    size_t offset = DC_ARENA_GEO_INDEX_TO_OFFSET(index.index, block, INITIAL_BLOCK_INDEX_BITS);

    if (block == self->block_current && offset >= self->block_current_exclusive_end) {
        return NULL;
    }

    SLOT* slot = &self->blocks[block][offset];
    if (!slot->present) {
        return NULL;
    }

    return &slot->value;
}

DC_PUBLIC static VALUE const* NS(SELF, read)(SELF const* self, INDEX index) {
    VALUE const* value = NS(SELF, try_read)(self, index);
    DC_ASSERT(value, "Cannot read item, index not found {index=%lu}", (size_t)index.index);
    return value;
}

DC_PUBLIC static VALUE* NS(SELF, try_write)(SELF* self, INDEX index) {
    return (VALUE*)NS(SELF, try_read)(self, index);
}

DC_PUBLIC static VALUE* NS(SELF, write)(SELF* self, INDEX index) {
    VALUE* value = NS(SELF, try_write)(self, index);
    DC_ASSERT(value, "Cannot write item, index not found {index=%lu}", (size_t)index.index);
    return value;
}

DC_PUBLIC static size_t NS(SELF, size)(SELF const* self) {
    INVARIANT_CHECK(self);
    return self->count;
}

DC_PUBLIC static SELF NS(SELF, clone)(SELF const* self) {
    INVARIANT_CHECK(self);

    SELF new_self = {
        .free_list = self->free_list,
        .count = self->count,
        .alloc_ref = self->alloc_ref,
        .derive_c_arena_blocks = dc_gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
        .block_current_exclusive_end = self->block_current_exclusive_end,
        .block_current = self->block_current,
        .blocks = {},
    };

    for (size_t block_index = 0; block_index <= self->block_current; block_index++) {
        size_t block_items = DC_ARENA_GEO_BLOCK_TO_SIZE(block_index, INITIAL_BLOCK_INDEX_BITS);
        SLOT* block_slots =
            (SLOT*)NS(ALLOC, allocate_uninit)(self->alloc_ref, block_items * sizeof(SLOT));
        new_self.blocks[block_index] = block_slots;

        size_t const to_offset =
            block_index == self->block_current ? self->block_current_exclusive_end : block_items;

        for (size_t offset = 0; offset < to_offset; offset++) {
            SLOT* src_slot = &self->blocks[block_index][offset];
            SLOT* dst_slot = &new_self.blocks[block_index][offset];
            NS(SLOT, clone_from)(src_slot, dst_slot);
        }
    }

    return new_self;
}

DC_PUBLIC static bool NS(SELF, try_remove)(SELF* self, INDEX index, VALUE* destination) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    uint8_t block = DC_ARENA_GEO_INDEX_TO_BLOCK(index.index, INITIAL_BLOCK_INDEX_BITS);
    if (block > self->block_current) {
        return false;
    }

    size_t offset = DC_ARENA_GEO_INDEX_TO_OFFSET(index.index, block, INITIAL_BLOCK_INDEX_BITS);

    if (block == self->block_current && offset >= self->block_current_exclusive_end) {
        return false;
    }

    SLOT* slot = &self->blocks[block][offset];
    if (slot->present) {
        *destination = slot->value;

        NS(SLOT, set_empty)(slot, self->free_list);
        self->free_list = index.index;
        self->count--;
        return true;
    }
    return false;
}

DC_PUBLIC static VALUE NS(SELF, remove)(SELF* self, INDEX index) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    VALUE value;
    DC_ASSERT(NS(SELF, try_remove)(self, index, &value),
              "Failed to remove item, index not found {index=%lu}", (size_t)index.index);
    return value;
}

DC_PUBLIC static void NS(SELF, delete)(SELF* self) {
    INVARIANT_CHECK(self);

    for (uint8_t block = 0; block <= self->block_current; block++) {
        size_t const to_offset = block == self->block_current
                                     ? self->block_current_exclusive_end
                                     : DC_ARENA_GEO_BLOCK_TO_SIZE(block, INITIAL_BLOCK_INDEX_BITS);

        for (size_t offset = 0; offset < to_offset; offset++) {
            SLOT* slot = &self->blocks[block][offset];
            if (slot->present) {
                VALUE_DELETE(&slot->value);
            }
        }

        size_t block_size =
            DC_ARENA_GEO_BLOCK_TO_SIZE(block, INITIAL_BLOCK_INDEX_BITS) * sizeof(SLOT);
        dc_memory_tracker_set(DC_MEMORY_TRACKER_LVL_CONTAINER, DC_MEMORY_TRACKER_CAP_WRITE,
                              self->blocks[block], block_size);
        NS(ALLOC, deallocate)(self->alloc_ref, self->blocks[block], block_size);
    }
}

#define IV_PAIR_CONST NS(SELF, iv_const)
typedef struct {
    INDEX index;
    VALUE const* value;
} IV_PAIR_CONST;

DC_PUBLIC static IV_PAIR_CONST NS(SELF, iv_const_empty)() {
    return (IV_PAIR_CONST){
        .index = {.index = INDEX_NONE},
        .value = NULL,
    };
}

#define ITER_CONST NS(SELF, iter_const)
typedef IV_PAIR_CONST NS(ITER_CONST, item);

DC_PUBLIC static bool NS(ITER_CONST, empty_item)(IV_PAIR_CONST const* item) {
    return item->value == NULL;
}

typedef struct {
    SELF const* arena;
    INDEX_TYPE next_index;
    mutation_version version;
} ITER_CONST;

DC_PUBLIC static IV_PAIR_CONST NS(ITER_CONST, next)(ITER_CONST* iter) {
    DC_ASSUME(iter);
    mutation_version_check(&iter->version);

    while (iter->next_index < MAX_INDEX) {
        uint8_t block = DC_ARENA_GEO_INDEX_TO_BLOCK(iter->next_index, INITIAL_BLOCK_INDEX_BITS);
        size_t offset =
            DC_ARENA_GEO_INDEX_TO_OFFSET(iter->next_index, block, INITIAL_BLOCK_INDEX_BITS);

        if ((block == iter->arena->block_current &&
             offset >= iter->arena->block_current_exclusive_end) ||
            (block > iter->arena->block_current)) {
            break;
        }

        SLOT* slot = &iter->arena->blocks[block][offset];
        if (slot->present) {
            IV_PAIR_CONST const result = {
                .index = (INDEX){.index = iter->next_index},
                .value = &slot->value,
            };
            iter->next_index++;
            return result;
        }

        iter->next_index++;
    }

    return NS(SELF, iv_const_empty)();
}

DC_PUBLIC static bool NS(ITER_CONST, empty)(ITER_CONST const* iter) {
    DC_ASSUME(iter);
    mutation_version_check(&iter->version);

    // Scan forward to see if there are any present slots remaining
    INDEX_TYPE search_index = iter->next_index;
    while (search_index < MAX_INDEX) {
        uint8_t block = DC_ARENA_GEO_INDEX_TO_BLOCK(search_index, INITIAL_BLOCK_INDEX_BITS);
        size_t offset = DC_ARENA_GEO_INDEX_TO_OFFSET(search_index, block, INITIAL_BLOCK_INDEX_BITS);

        // Check if we're past the valid range
        if ((block == iter->arena->block_current &&
             offset >= iter->arena->block_current_exclusive_end) ||
            (block > iter->arena->block_current)) {
            return true;
        }

        SLOT* slot = &iter->arena->blocks[block][offset];
        if (slot->present) {
            return false;
        }

        search_index++;
    }

    return true;
}

DC_PUBLIC static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    INVARIANT_CHECK(self);

    return (ITER_CONST){
        .arena = self,
        .next_index = 0,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

DC_PUBLIC static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, DC_EXPAND_STRING(SELF) "@%p {\n", (void*)self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "count: %lu,\n", self->count);

    if (self->free_list == INDEX_NONE) {
        dc_debug_fmt_print(fmt, stream, "free_list: INDEX_NONE,\n");
    } else {
        dc_debug_fmt_print(fmt, stream, "free_list: %lu,\n", (size_t)self->free_list);
    }

    dc_debug_fmt_print(fmt, stream, "alloc: ");
    NS(ALLOC, debug)(NS(NS(ALLOC, ref), deref)(self->alloc_ref), fmt, stream);
    fprintf(stream, ",\n");

    dc_debug_fmt_print(fmt, stream, "blocks: [\n");
    fmt = dc_debug_fmt_scope_begin(fmt);
    for (size_t block = 0; block <= self->block_current; block++) {
        dc_debug_fmt_print(fmt, stream, "{\n");
        fmt = dc_debug_fmt_scope_begin(fmt);

        size_t const capacity = DC_ARENA_GEO_BLOCK_TO_SIZE(block, INITIAL_BLOCK_INDEX_BITS);
        size_t const to_offset =
            block == self->block_current ? self->block_current_exclusive_end : capacity;

        dc_debug_fmt_print(fmt, stream, "block_index: %lu,\n", block);
        dc_debug_fmt_print(fmt, stream, "block_ptr: %p,\n", (void*)self->blocks[block]);
        dc_debug_fmt_print(fmt, stream, "capacity: %lu,\n", capacity);
        dc_debug_fmt_print(fmt, stream, "size: %lu,\n", to_offset);
        dc_debug_fmt_print(fmt, stream, "slots: [\n");
        fmt = dc_debug_fmt_scope_begin(fmt);

        for (size_t offset = 0; offset < to_offset; offset++) {
            SLOT* slot = &self->blocks[block][offset];
            dc_debug_fmt_print(fmt, stream, "{\n");
            fmt = dc_debug_fmt_scope_begin(fmt);

            dc_debug_fmt_print(fmt, stream, "present: %s,\n", slot->present ? "true" : "false");
            if (slot->present) {
                dc_debug_fmt_print(fmt, stream, "value: ");
                VALUE_DEBUG(&slot->value, fmt, stream);
                fprintf(stream, ",\n");
            } else {
                dc_debug_fmt_print(fmt, stream, "next_free: %lu,\n", (size_t)slot->next_free);
            }

            fmt = dc_debug_fmt_scope_end(fmt);
            dc_debug_fmt_print(fmt, stream, "},\n");
        }

        fmt = dc_debug_fmt_scope_end(fmt);
        dc_debug_fmt_print(fmt, stream, "],\n");

        /* Close the block's scope and print its closing brace */
        fmt = dc_debug_fmt_scope_end(fmt);
        dc_debug_fmt_print(fmt, stream, "},\n");
    }

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "],\n");

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#undef ITER_CONST
#undef IV_PAIR_CONST

#define IV_PAIR NS(SELF, iv)
typedef struct {
    INDEX index;
    VALUE const* value;
} IV_PAIR;

DC_PUBLIC static IV_PAIR NS(SELF, iv_empty)() {
    return (IV_PAIR){
        .index = {.index = INDEX_NONE},
        .value = NULL,
    };
}

#define ITER NS(SELF, iter)
typedef IV_PAIR NS(ITER, item);

DC_PUBLIC static bool NS(ITER, empty_item)(IV_PAIR const* item) { return item->value == NULL; }

typedef struct {
    SELF* arena;
    INDEX_TYPE next_index;
    mutation_version version;
} ITER;

DC_PUBLIC static IV_PAIR NS(ITER, next)(ITER* iter) {
    DC_ASSUME(iter);
    mutation_version_check(&iter->version);

    while (iter->next_index < MAX_INDEX) {
        uint8_t block = DC_ARENA_GEO_INDEX_TO_BLOCK(iter->next_index, INITIAL_BLOCK_INDEX_BITS);
        size_t offset =
            DC_ARENA_GEO_INDEX_TO_OFFSET(iter->next_index, block, INITIAL_BLOCK_INDEX_BITS);

        if ((block == iter->arena->block_current &&
             offset >= iter->arena->block_current_exclusive_end) ||
            (block > iter->arena->block_current)) {
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

    return NS(SELF, iv_empty)();
}

DC_PUBLIC static bool NS(ITER, empty)(ITER const* iter) {
    DC_ASSUME(iter);
    mutation_version_check(&iter->version);

    // Scan forward to see if there are any present slots remaining
    INDEX_TYPE search_index = iter->next_index;
    while (search_index < MAX_INDEX) {
        uint8_t block = DC_ARENA_GEO_INDEX_TO_BLOCK(search_index, INITIAL_BLOCK_INDEX_BITS);
        size_t offset = DC_ARENA_GEO_INDEX_TO_OFFSET(search_index, block, INITIAL_BLOCK_INDEX_BITS);

        // Check if we're past the valid range
        if ((block == iter->arena->block_current &&
             offset >= iter->arena->block_current_exclusive_end) ||
            (block > iter->arena->block_current)) {
            return true;
        }

        SLOT* slot = &iter->arena->blocks[block][offset];
        if (slot->present) {
            return false;
        }

        search_index++;
    }

    return true;
}

DC_PUBLIC static ITER NS(SELF, get_iter)(SELF* self) {
    INVARIANT_CHECK(self);

    return (ITER){
        .arena = self,
        .next_index = 0,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

#undef ITER
#undef IV_PAIR
#undef INVARIANT_CHECK
#undef SLOT
#undef INITIAL_BLOCK_INDEX_BITS

#include <derive-c/core/index/type_to_strong/undef.h>
#include <derive-c/core/index/bits_to_type/undef.h>

#undef VALUE_DEBUG
#undef VALUE_CLONE
#undef VALUE_DELETE
#undef VALUE
#undef INDEX_BITS

DC_TRAIT_ARENA(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/includes/undef.h>
