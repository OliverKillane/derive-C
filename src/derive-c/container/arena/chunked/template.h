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
    #define INDEX_BITS 8
#endif

#if !defined BLOCK_INDEX_BITS
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("The number of bits used to get the offset within a block must be specified")
    #endif
    #define BLOCK_INDEX_BITS 7
#endif

DC_STATIC_ASSERT(BLOCK_INDEX_BITS > 0, "Cannot have zero block index bits");
DC_STATIC_ASSERT(INDEX_BITS > 0, "Cannot have zero index bits");
DC_STATIC_ASSERT(INDEX_BITS > BLOCK_INDEX_BITS,
                 "The number of bits for offset within a block must be "
                 "less than the number of bits used for an index");

#if !defined VALUE
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("The value type to place in the arena must be defined")
    #endif
    #define VALUE value_t
typedef struct {
    int x;
} VALUE;
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

typedef VALUE NS(SELF, value_t);
typedef ALLOC NS(SELF, alloc_t);

#define SLOT NS(NAME, slot)

#define SLOT_INDEX_TYPE INDEX_TYPE     // [DERIVE-C] for template
#define SLOT_VALUE VALUE               // [DERIVE-C] for template
#define SLOT_VALUE_CLONE VALUE_CLONE   // [DERIVE-C] for template
#define SLOT_VALUE_CLONE VALUE_CLONE   // [DERIVE-C] for template
#define SLOT_VALUE_DELETE VALUE_DELETE // [DERIVE-C] for template
#define INTERNAL_NAME SLOT             // [DERIVE-C] for template
#include <derive-c/utils/slot/template.h>

typedef SLOT PRIV(NS(SELF, block))[DC_ARENA_CHUNKED_BLOCK_SIZE(BLOCK_INDEX_BITS)];

typedef struct {
    size_t count;
    INDEX_TYPE free_list;

    PRIV(NS(SELF, block)) * *blocks;
    INDEX_TYPE block_current;
    INDEX_TYPE block_current_exclusive_end;

    NS(ALLOC, ref) alloc_ref;
    dc_gdb_marker derive_c_arena_basic;
    mutation_tracker iterator_invalidation_tracker;
} SELF;

#define INVARIANT_CHECK(self)                                                                      \
    DC_ASSUME(self);                                                                               \
    DC_ASSUME(((self))->count <= MAX_INDEX);                                                       \
    DC_ASSUME(((self)->block_current_exclusive_end) <=                                             \
              DC_ARENA_CHUNKED_BLOCK_SIZE(BLOCK_INDEX_BITS));                                      \
    DC_ASSUME(DC_WHEN((self)->free_list == INDEX_NONE,                                             \
                      (self)->count ==                                                             \
                          (DC_ARENA_CHUNKED_BLOCK_SIZE(BLOCK_INDEX_BITS) * (self)->block_current + \
                           (self)->block_current_exclusive_end)),                                  \
              "All slots are full if the free list is empty");

DC_PUBLIC static SELF NS(SELF, new)(NS(ALLOC, ref) alloc_ref) {
    PRIV(NS(SELF, block))* first_block = (PRIV(NS(SELF, block))*)NS(ALLOC, allocate_uninit)(
        alloc_ref, sizeof(PRIV(NS(SELF, block))));
    PRIV(NS(SELF, block))** blocks = (PRIV(NS(SELF, block))**)NS(ALLOC, allocate_uninit)(
        alloc_ref, sizeof(PRIV(NS(SELF, block))*));

    blocks[0] = first_block;

    for (INDEX_TYPE offset = 0; offset < DC_ARENA_CHUNKED_BLOCK_SIZE(BLOCK_INDEX_BITS); offset++) {
        /* Properly index the slots within the allocated block */
        NS(SLOT, memory_tracker_empty)(&(*first_block)[offset]);
    }

    return (SELF){
        .count = 0,
        .free_list = INDEX_NONE,
        .blocks = blocks,
        .block_current = 0,
        .block_current_exclusive_end = 0,
        .alloc_ref = alloc_ref,
        .derive_c_arena_basic = dc_gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

DC_PUBLIC static INDEX NS(SELF, insert)(SELF* self, VALUE value) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);
    DC_ASSERT(self->count < MAX_INDEX);

    if (self->free_list != INDEX_NONE) {
        INDEX_TYPE free_index = self->free_list;
        INDEX_TYPE block = DC_ARENA_CHUNKED_INDEX_TO_BLOCK(free_index, BLOCK_INDEX_BITS);
        INDEX_TYPE offset = DC_ARENA_CHUNKED_INDEX_TO_OFFSET(free_index, BLOCK_INDEX_BITS);

        SLOT* slot = &(*self->blocks[block])[offset];

        DC_ASSUME(!slot->present);
        self->free_list = slot->next_free;

        NS(SLOT, fill)(slot, value);

        self->count++;
        return (INDEX){.index = free_index};
    }

    if (self->block_current_exclusive_end == DC_ARENA_CHUNKED_BLOCK_SIZE(BLOCK_INDEX_BITS)) {
        self->block_current++;
        self->block_current_exclusive_end = 0;

        size_t blocks_current_size = self->block_current * sizeof(PRIV(NS(SELF, block))*);
        size_t blocks_new_size = blocks_current_size + sizeof(PRIV(NS(SELF, block))*);

        self->blocks = (PRIV(NS(SELF, block))**)NS(ALLOC, reallocate)(
            self->alloc_ref, (void*)self->blocks, blocks_current_size, blocks_new_size);

        PRIV(NS(SELF, block))* new_block = (PRIV(NS(SELF, block))*)NS(ALLOC, allocate_uninit)(
            self->alloc_ref, sizeof(PRIV(NS(SELF, block))));

        self->blocks[self->block_current] = new_block;

        for (size_t offset = 0; offset < DC_ARENA_CHUNKED_BLOCK_SIZE(BLOCK_INDEX_BITS); offset++) {
            NS(SLOT, memory_tracker_empty)(&(*new_block)[offset]);
        }
    }

    SLOT* slot = &(*self->blocks[self->block_current])[self->block_current_exclusive_end];
    NS(SLOT, fill)(slot, value);

    INDEX_TYPE index = (INDEX_TYPE)DC_ARENA_CHUNKED_BLOCK_OFFSET_TO_INDEX(
        self->block_current, self->block_current_exclusive_end, BLOCK_INDEX_BITS);
    self->count++;
    self->block_current_exclusive_end++;

    return (INDEX){.index = index};
}

DC_PUBLIC static VALUE const* NS(SELF, try_read)(SELF const* self, INDEX index) {
    INVARIANT_CHECK(self);

    INDEX_TYPE block = DC_ARENA_CHUNKED_INDEX_TO_BLOCK(index.index, BLOCK_INDEX_BITS);
    INDEX_TYPE offset = DC_ARENA_CHUNKED_INDEX_TO_OFFSET(index.index, BLOCK_INDEX_BITS);

    if (block > self->block_current ||
        (block == self->block_current && offset >= self->block_current_exclusive_end)) {
        return NULL;
    }

    SLOT* slot = &(*self->blocks[block])[offset];

    if (!slot->present) {
        return NULL;
    }
    return &slot->value;
}

DC_PUBLIC static VALUE const* NS(SELF, read)(SELF const* self, INDEX index) {
    VALUE const* value = NS(SELF, try_read)(self, index);
    DC_ASSERT(value);
    return value;
}

DC_PUBLIC static VALUE* NS(SELF, try_write)(SELF* self, INDEX index) {
    return (VALUE*)NS(SELF, try_read)(self, index);
}

DC_PUBLIC static VALUE* NS(SELF, write)(SELF* self, INDEX index) {
    VALUE* value = NS(SELF, try_write)(self, index);
    DC_ASSERT(value);
    return value;
}

DC_PUBLIC static SELF NS(SELF, clone)(SELF const* self) {
    INVARIANT_CHECK(self);

    PRIV(NS(SELF, block))** blocks = (PRIV(NS(SELF, block))**)NS(ALLOC, allocate_uninit)(
        self->alloc_ref, sizeof(PRIV(NS(SELF, block))*) * (self->block_current + 1));

    for (INDEX_TYPE b = 0; b <= self->block_current; b++) {
        blocks[b] = (PRIV(NS(SELF, block))*)NS(ALLOC, allocate_uninit)(
            self->alloc_ref, sizeof(PRIV(NS(SELF, block))));
    }

    for (INDEX_TYPE b = 0; b < self->block_current; b++) {
        PRIV(NS(SELF, block))* to_block = blocks[b];
        PRIV(NS(SELF, block)) const* from_block = self->blocks[b];
        for (INDEX_TYPE i = 0; i < DC_ARENA_CHUNKED_BLOCK_SIZE(BLOCK_INDEX_BITS); i++) {
            NS(SLOT, clone_from)(&(*from_block)[i], &(*to_block)[i]);
        }
    }

    PRIV(NS(SELF, block))* to_current_block = blocks[self->block_current];
    PRIV(NS(SELF, block)) const* from_current_block = self->blocks[self->block_current];
    for (INDEX_TYPE i = 0; i < self->block_current_exclusive_end; i++) {
        NS(SLOT, clone_from)(&(*from_current_block)[i], &(*to_current_block)[i]);
    }

    return (SELF){
        .count = self->count,
        .free_list = self->free_list,
        .blocks = blocks,
        .block_current = self->block_current,
        .block_current_exclusive_end = self->block_current_exclusive_end,
        .alloc_ref = self->alloc_ref,
        .derive_c_arena_basic = dc_gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

DC_PUBLIC static size_t NS(SELF, size)(SELF const* self) {
    INVARIANT_CHECK(self);
    return self->count;
}

DC_PUBLIC static bool NS(SELF, full)(SELF const* self) {
    INVARIANT_CHECK(self);
    return self->count < MAX_INDEX;
}

DC_PUBLIC static const size_t NS(SELF, max_entries) = MAX_INDEX;

DC_PUBLIC static bool NS(SELF, try_remove)(SELF* self, INDEX index, VALUE* destination) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    INDEX_TYPE block = DC_ARENA_CHUNKED_INDEX_TO_BLOCK(index.index, BLOCK_INDEX_BITS);
    INDEX_TYPE offset = DC_ARENA_CHUNKED_INDEX_TO_OFFSET(index.index, BLOCK_INDEX_BITS);

    /* Only treat offset vs block_current_exclusive_end for the last block */
    if (block > self->block_current ||
        (block == self->block_current && offset >= self->block_current_exclusive_end)) {
        return false;
    }

    PRIV(NS(SELF, block))* current_block = self->blocks[block];
    SLOT* entry = &(*current_block)[offset];

    if (entry->present) {
        *destination = entry->value;

        NS(SLOT, set_empty)(entry, self->free_list);

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
    DC_ASSERT(NS(SELF, try_remove)(self, index, &value));
    return value;
}

DC_PUBLIC static INDEX_TYPE PRIV(NS(SELF, next_index_value))(SELF const* self,
                                                             INDEX_TYPE from_index) {
    for (INDEX_TYPE next_index = from_index + 1;; next_index++) {
        INDEX_TYPE block = DC_ARENA_CHUNKED_INDEX_TO_BLOCK(next_index, BLOCK_INDEX_BITS);
        INDEX_TYPE offset = DC_ARENA_CHUNKED_INDEX_TO_OFFSET(next_index, BLOCK_INDEX_BITS);

        if (block > self->block_current ||
            (block == self->block_current && offset >= self->block_current_exclusive_end)) {
            return INDEX_NONE;
        }

        /* Fix wrong indexing: use &(*self->blocks[block])[offset] rather than
           self->blocks[block][offset] which indexes by block-sized strides. */
        SLOT* slot = &(*self->blocks[block])[offset];
        if (slot->present) {
            return next_index;
        }
    }
}

#define ITER NS(SELF, iter)
#define IV_PAIR NS(ITER, item)

typedef struct {
    SELF* arena;
    INDEX_TYPE next_index;
    mutation_version version;
} ITER;

#define ITER_INVARIANT_CHECK(iter)                                                                 \
    DC_ASSUME(iter);                                                                               \
    DC_DEBUG_ASSERT(                                                                               \
        DC_WHEN((iter)->next_index != INDEX_NONE,                                                  \
                NS(SELF, try_read)(iter->arena, (INDEX){.index = (iter)->next_index}) != NULL),    \
        "The next index is either valid, or the iterator is empty");

typedef struct {
    INDEX index;
    VALUE* value;
} IV_PAIR;

DC_PUBLIC static IV_PAIR NS(SELF, iv_empty)() {
    return (IV_PAIR){.index = (INDEX){.index = INDEX_NONE}, .value = NULL};
}

DC_PUBLIC static bool NS(ITER, empty_item)(IV_PAIR const* item) { return item->value == NULL; }

DC_PUBLIC static bool NS(ITER, empty)(ITER const* iter) {
    ITER_INVARIANT_CHECK(iter);
    mutation_version_check(&iter->version);
    return iter->next_index == INDEX_NONE;
}

DC_PUBLIC static IV_PAIR NS(ITER, next)(ITER* iter) {
    ITER_INVARIANT_CHECK(iter);
    mutation_version_check(&iter->version);

    if (iter->next_index == INDEX_NONE) {
        return NS(SELF, iv_empty)();
    }

    INDEX index = {.index = iter->next_index};
    IV_PAIR result = (IV_PAIR){
        .index = index,
        .value = NS(SELF, write)(iter->arena, index),
    };

    iter->next_index = PRIV(NS(SELF, next_index_value))(iter->arena, iter->next_index);
    return result;
}

DC_PUBLIC static ITER NS(SELF, get_iter)(SELF* self) {
    INVARIANT_CHECK(self);
    
    // Check if index 0 is present, otherwise find the next valid index
    INDEX_TYPE first_index;
    if (self->block_current_exclusive_end > 0 && (*self->blocks[0])[0].present) {
        first_index = 0;
    } else {
        first_index = PRIV(NS(SELF, next_index_value))(self, 0);
    }
    
    return (ITER){
        .arena = self,
        .next_index = first_index,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

DC_PUBLIC static void NS(SELF, delete)(SELF* self) {
    INVARIANT_CHECK(self);
    ITER iter = NS(SELF, get_iter)(self);

    for (IV_PAIR entry = NS(ITER, next)(&iter); !NS(ITER, empty_item)(&entry);
         entry = NS(ITER, next)(&iter)) {
        VALUE_DELETE(entry.value);
    }

    for (INDEX_TYPE b = 0; b <= self->block_current; b++) {
        NS(ALLOC, deallocate)(self->alloc_ref, self->blocks[b], sizeof(PRIV(NS(SELF, block))));
    }
    NS(ALLOC, deallocate)(self->alloc_ref, (void*)self->blocks,
                          self->block_current * sizeof(PRIV(NS(SELF, block))*));
}

#undef ITER_INVARIANT_CHECK
#undef IV_PAIR
#undef ITER

#define ITER_CONST NS(SELF, iter_const)
#define IV_PAIR_CONST NS(ITER_CONST, item)

typedef struct {
    SELF const* arena;
    INDEX_TYPE next_index;
    mutation_version version;
} ITER_CONST;

#define ITER_CONST_INVARIANT_CHECK(iter)                                                           \
    DC_ASSUME(iter);                                                                               \
    DC_DEBUG_ASSERT(                                                                               \
        DC_WHEN((iter)->next_index != INDEX_NONE,                                                  \
                NS(SELF, try_read)(iter->arena, (INDEX){.index = (iter)->next_index}) != NULL),    \
        "The next index is either valid, or the iterator is empty");

typedef struct {
    INDEX index;
    VALUE const* value;
} IV_PAIR_CONST;

DC_PUBLIC static IV_PAIR_CONST NS(SELF, iv_const_empty)() {
    return (IV_PAIR_CONST){.index = (INDEX){.index = INDEX_NONE}, .value = NULL};
}

DC_PUBLIC static bool NS(ITER_CONST, empty_item)(IV_PAIR_CONST const* item) {
    return item->value == NULL;
}

DC_PUBLIC static bool NS(ITER_CONST, empty)(ITER_CONST const* iter) {
    ITER_CONST_INVARIANT_CHECK(iter);
    mutation_version_check(&iter->version);
    return iter->next_index == INDEX_NONE;
}

DC_PUBLIC static IV_PAIR_CONST NS(ITER_CONST, next)(ITER_CONST* iter) {
    ITER_CONST_INVARIANT_CHECK(iter);
    mutation_version_check(&iter->version);

    if (iter->next_index == INDEX_NONE) {
        return NS(SELF, iv_const_empty)();
    }

    INDEX index = {.index = iter->next_index};
    IV_PAIR_CONST result = (IV_PAIR_CONST){
        .index = index,
        .value = NS(SELF, read)(iter->arena, index),
    };

    iter->next_index = PRIV(NS(SELF, next_index_value))(iter->arena, iter->next_index);
    return result;
}

DC_PUBLIC static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    INVARIANT_CHECK(self);
    
    // Check if index 0 is present, otherwise find the next valid index
    INDEX_TYPE first_index;
    if (self->block_current_exclusive_end > 0 && (*self->blocks[0])[0].present) {
        first_index = 0;
    } else {
        first_index = PRIV(NS(SELF, next_index_value))(self, 0);
    }
    
    return (ITER_CONST){
        .arena = self,
        .next_index = first_index,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

DC_PUBLIC static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, DC_EXPAND_STRING(SELF) "@%p {\n", (void*)self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "count: %lu,\n", self->count);
    dc_debug_fmt_print(fmt, stream, "free_list: %lu,\n", (size_t)self->free_list);

    dc_debug_fmt_print(fmt, stream, "alloc: ");
    NS(ALLOC, debug)(NS(NS(ALLOC, ref), deref)(self->alloc_ref), fmt, stream);
    fprintf(stream, ",\n");

    dc_debug_fmt_print(fmt, stream, "current_block: %lu\n", (size_t)self->block_current);
    dc_debug_fmt_print(fmt, stream, "block_current_exclusive_end: %lu\n",
                       (size_t)self->block_current_exclusive_end);
    dc_debug_fmt_print(fmt, stream, "blocks: [\n");
    fmt = dc_debug_fmt_scope_begin(fmt);

    for (INDEX_TYPE b = 0; b <= self->block_current; b++) {

        dc_debug_fmt_print(fmt, stream, "block[%lu]: @%p [\n", (size_t)b, (void*)self->blocks[b]);
        fmt = dc_debug_fmt_scope_begin(fmt);

        INDEX_TYPE block_entry_exclusive_end = b == self->block_current
                                                   ? self->block_current_exclusive_end
                                                   : DC_ARENA_CHUNKED_BLOCK_SIZE(BLOCK_INDEX_BITS);

        for (INDEX_TYPE i = 0; i < block_entry_exclusive_end; i++) {
            /* Previously used self->blocks[b][i] which computes wrong address.
               Use the dereference-then-index form to get the SLOT. */
            SLOT* entry = &(*self->blocks[b])[i];

            if (entry->present) {
                dc_debug_fmt_print(
                    fmt, stream, "[index=%lu] ",
                    (size_t)DC_ARENA_CHUNKED_BLOCK_OFFSET_TO_INDEX(b, i, BLOCK_INDEX_BITS));
                VALUE_DEBUG(&entry->value, fmt, stream);
                fprintf(stream, ",\n");
            } else {
                dc_debug_fmt_print(
                    fmt, stream, "[index=%lu]{ next_free=%lu }\n",
                    (size_t)DC_ARENA_CHUNKED_BLOCK_OFFSET_TO_INDEX(b, i, BLOCK_INDEX_BITS),
                    (size_t)entry->next_free);
            }
        }

        fmt = dc_debug_fmt_scope_end(fmt);
        dc_debug_fmt_print(fmt, stream, "],\n");
    }

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "],\n");

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#undef ITER_CONST_INVARIANT_CHECK
#undef IV_PAIR_CONST
#undef ITER_CONST

#undef INVARIANT_CHECK
#undef SLOT

#include <derive-c/core/index/type_to_strong/undef.h>
#include <derive-c/core/index/bits_to_type/undef.h>

#undef VALUE_DEBUG
#undef VALUE_CLONE
#undef VALUE_DELETE
#undef VALUE

#undef BLOCK_INDEX_BITS
#undef INDEX_BITS

DC_TRAIT_ARENA(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/includes/undef.h>
