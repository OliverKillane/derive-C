/// @brief A simple bitset for indexes `[0, EXCLUSIVE_END_INDEX)`, statically allocated.

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/self/def.h>

#if !defined EXCLUSIVE_END_INDEX
    #if !defined DC_PLACEHOLDERS
TEMPLATE_ERROR("no EXCLUSIVE_END_INDEX")
    #endif
    #define EXCLUSIVE_END_INDEX 32
#endif

DC_STATIC_ASSERT(
    EXCLUSIVE_END_INDEX > 0,
    DC_EXPAND_STRING(SELF) " EXCLUSIVE_END_INDEX must be larger than 0 for nonempty bitset");

#define INDICES_CAPACITY EXCLUSIVE_END_INDEX

#include <derive-c/core/index/capacity_to_bits/def.h>
#include <derive-c/core/index/bits_to_type/def.h>

typedef INDEX_TYPE NS(SELF, index_t);

DC_STATIC_CONSTANT INDEX_TYPE NS(SELF, max_index) = EXCLUSIVE_END_INDEX - 1;
DC_STATIC_CONSTANT INDEX_TYPE NS(SELF, min_index) = 0;

typedef struct {
    uint8_t bits[DC_BITSET_STATIC_CAPACITY_TO_BYTES(EXCLUSIVE_END_INDEX)];
    dc_gdb_marker derive_c_bitset_static;
    mutation_tracker iterator_invalidation_tracker;
} SELF;

#define INVARIANT_CHECK(self) DC_ASSUME(self);

static SELF NS(SELF, new)() {
    return (SELF){
        .bits = {},
        .derive_c_bitset_static = dc_gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static bool NS(SELF, try_set)(SELF* self, INDEX_TYPE index, bool value) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    // JUSTIFY: Only checking if the end index could be smaller than max
    //  - If exclusive end is 1 larger than the max index, this check does not work.
    //    e.g. exclusive end is 256. So index is uint8_t, which means we can never represent an
    //         incorrect index, so no check.
#if EXCLUSIVE_END_INDEX <= MAX_INDEX
    if (index >= EXCLUSIVE_END_INDEX) {
        return false;
    }
#endif

    INDEX_TYPE byte = DC_BITSET_STATIC_INDEX_TO_BYTES(index);
    INDEX_TYPE offset = DC_BITSET_STATIC_INDEX_TO_OFFSET(index);
    uint8_t mask = DC_BITSET_STATIC_OFFSET_TO_MASK(offset);

    if (value) {
        self->bits[byte] = self->bits[byte] | mask;
    } else {
        self->bits[byte] = self->bits[byte] & (~mask);
    }
    return true;
}

static void NS(SELF, set)(SELF* self, INDEX_TYPE index, bool value) {
    INVARIANT_CHECK(self);
    DC_ASSERT(NS(SELF, try_set)(self, index, value));
}

static bool NS(SELF, get)(SELF const* self, INDEX_TYPE index) {
    INVARIANT_CHECK(self);

#if EXCLUSIVE_END_INDEX <= MAX_INDEX
    DC_ASSERT(index < EXCLUSIVE_END_INDEX);
#endif

    INDEX_TYPE byte = DC_BITSET_STATIC_INDEX_TO_BYTES(index);
    INDEX_TYPE offset = DC_BITSET_STATIC_INDEX_TO_OFFSET(index);
    uint8_t mask = DC_BITSET_STATIC_OFFSET_TO_MASK(offset);
    return (self->bits[byte] & mask) != 0;
}

static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, DC_EXPAND_STRING(SELF) "@%p {\n", self);
    fmt = dc_debug_fmt_scope_begin(fmt);

    dc_debug_fmt_print(fmt, stream, "blocks: [\n");
    fmt = dc_debug_fmt_scope_begin(fmt);
    for (size_t index = 0; index < EXCLUSIVE_END_INDEX; index++) {
        if (NS(SELF, get)(self, (INDEX_TYPE)index)) {
            dc_debug_fmt_print(fmt, stream, "{ byte: %lu, offset: %lu, index: %lu},\n",
                               DC_BITSET_STATIC_INDEX_TO_BYTES(index),
                               DC_BITSET_STATIC_INDEX_TO_OFFSET(index), index);
        }
    }
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "],\n");

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

static SELF NS(SELF, clone)(SELF const* self) {
    INVARIANT_CHECK(self);
    SELF new_self = (SELF){
        .derive_c_bitset_static = dc_gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
    memcpy(&new_self.bits, &self->bits, sizeof(self->bits));
    return new_self;
}

static size_t NS(SELF, size)(SELF const* self) {
    size_t size = 0;

    for (INDEX_TYPE byte = 0;
         byte < (INDEX_TYPE)DC_BITSET_STATIC_CAPACITY_TO_BYTES(EXCLUSIVE_END_INDEX); byte++) {
        size += (uint32_t)__builtin_popcount((unsigned int)self->bits[byte]);
    }

    return size;
}

static void NS(SELF, delete)(SELF* self) { INVARIANT_CHECK(self); }

// JUSTIFY: Larger iter index type if the exclusive end is larger than the max representable index.
//  - We need to represent the none index.
//  - We need the smallest type that can represent the exclusive end index.
#if EXCLUSIVE_END_INDEX < MAX_INDEX
    #define ITER_INDEX_TYPE INDEX_TYPE
#else
    #define ITER_INDEX_TYPE INDEX_LARGER_TYPE
#endif

#define ITER_CONST NS(SELF, iter_const)
typedef struct {
    SELF const* bitset;
    ITER_INDEX_TYPE next_index;
    mutation_version version;
} ITER_CONST;
typedef ITER_INDEX_TYPE NS(ITER_CONST, item);

static bool NS(ITER_CONST, empty_item)(ITER_INDEX_TYPE const* item) {
    return *item == EXCLUSIVE_END_INDEX;
}

static ITER_INDEX_TYPE NS(ITER_CONST, next)(ITER_CONST* iter) {
    DC_ASSUME(iter);
    mutation_version_check(&iter->version);

    if (iter->next_index == EXCLUSIVE_END_INDEX) {
        return EXCLUSIVE_END_INDEX;
    }

    ITER_INDEX_TYPE next_index = iter->next_index;
    iter->next_index++;
    while (iter->next_index < EXCLUSIVE_END_INDEX &&
           !NS(SELF, get)(iter->bitset, (INDEX_TYPE)iter->next_index)) {
        iter->next_index++;
    }
    return next_index;
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    INVARIANT_CHECK(self);

    ITER_INDEX_TYPE next_index = 0;
    while (next_index < EXCLUSIVE_END_INDEX && !NS(SELF, get)(self, (INDEX_TYPE)next_index)) {
        next_index++;
    }

    return (ITER_CONST){
        .bitset = self,
        .next_index = next_index,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

#undef ITER_CONST

#define ITER NS(SELF, iter)
typedef struct {
    SELF* bitset;
    ITER_INDEX_TYPE next_index;
    mutation_version version;
} ITER;
typedef ITER_INDEX_TYPE NS(ITER, item);

static bool NS(ITER, empty_item)(ITER_INDEX_TYPE const* item) {
    return *item == EXCLUSIVE_END_INDEX;
}

static ITER_INDEX_TYPE NS(ITER, next)(ITER* iter) {
    DC_ASSUME(iter);
    mutation_version_check(&iter->version);

    if (iter->next_index == EXCLUSIVE_END_INDEX) {
        return EXCLUSIVE_END_INDEX;
    }

    ITER_INDEX_TYPE next_index = iter->next_index;
    iter->next_index++;
    while (iter->next_index < EXCLUSIVE_END_INDEX &&
           !NS(SELF, get)(iter->bitset, (INDEX_TYPE)iter->next_index)) {
        iter->next_index++;
    }
    return next_index;
}

static ITER NS(SELF, get_iter)(SELF* self) {
    INVARIANT_CHECK(self);

    ITER_INDEX_TYPE next_index = 0;
    while (next_index < EXCLUSIVE_END_INDEX && !NS(SELF, get)(self, (INDEX_TYPE)next_index)) {
        next_index++;
    }

    return (ITER){
        .bitset = self,
        .next_index = next_index,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

#undef ITER
#undef ITER_INDEX_TYPE
#undef INVARIANT_CHECK

#include <derive-c/core/index/bits_to_type/undef.h>
#include <derive-c/core/index/capacity_to_bits/undef.h>

#undef INDICES_CAPACITY
#undef EXCLUSIVE_END_INDEX

DC_TRAIT_BITSET(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/includes/undef.h>
