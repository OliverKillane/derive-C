/// @brief A simple bitset for indexes `[0, EXCLUSIVE_END_INDEX)`, statically allocated.

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/self/def.h>

#if !defined EXCLUSIVE_END_INDEX
    #if !defined PLACEHOLDERS
TEMPLATE_ERROR("no EXCLUSIVE_END_INDEX")
    #endif
    #define EXCLUSIVE_END_INDEX 32
#endif

// JUSTIFY: exclusive end + 1
//  - We want to use the smallest allowed type for indices
//  - We need to ake sure the EXCLUSIVE_END_INDEX can be represented by the type chosen.
//
// For example, with an EXCLUSIVE_END_INDEX of 255:
//  - Choose a uint8_t
// For example, with an EXCLUSIVE_END_INDEX of 256:
//  - Choose a uint16_t, so we can do index < EXCLUSIVE_END_INDEX
//  - Otherwise out for(..; index < EXCLUSIVE_END_INDEX; index++) loops would hit 255, then loop
//  back to zero and never complete.
#define INDICES_CAPACITY (EXCLUSIVE_END_INDEX + 1)

#include <derive-c/core/index/capacity_to_bits/def.h>
#include <derive-c/core/index/bits_to_type/def.h>

typedef INDEX_TYPE NS(SELF, index_t);

static INDEX_TYPE NS(SELF, max_index)() { return EXCLUSIVE_END_INDEX - 1; }

static INDEX_TYPE NS(SELF, min_index)() { return 0; }

typedef struct {
    uint8_t bits[DC_BITSET_STATIC_CAPACITY_TO_BYTES(EXCLUSIVE_END_INDEX)];
    gdb_marker derive_c_bitset_static;
    mutation_tracker iterator_invalidation_tracker;
} SELF;

#define INVARIANT_CHECK(self) ASSUME(self);

static SELF NS(SELF, new)() {
    return (SELF){
        .bits = {},
        .derive_c_bitset_static = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
}

static bool NS(SELF, try_set)(SELF* self, INDEX_TYPE index, bool value) {
    INVARIANT_CHECK(self);
    mutation_tracker_mutate(&self->iterator_invalidation_tracker);

    if (index >= EXCLUSIVE_END_INDEX) {
        return false;
    }

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
    ASSERT(NS(SELF, try_set)(self, index, value));
}

static bool NS(SELF, get)(SELF const* self, INDEX_TYPE index) {
    INVARIANT_CHECK(self);

    ASSERT(index < EXCLUSIVE_END_INDEX);

    INDEX_TYPE byte = DC_BITSET_STATIC_INDEX_TO_BYTES(index);
    INDEX_TYPE offset = DC_BITSET_STATIC_INDEX_TO_OFFSET(index);
    uint8_t mask = DC_BITSET_STATIC_OFFSET_TO_MASK(offset);
    return (self->bits[byte] & mask) != 0;
}

static void NS(SELF, debug)(SELF const* self, debug_fmt fmt, FILE* stream) {
    fprintf(stream, EXPAND_STRING(SELF) "@%p {\n", self);
    fmt = debug_fmt_scope_begin(fmt);

    debug_fmt_print(fmt, stream, "blocks: [");
    fmt = debug_fmt_scope_begin(fmt);
    for (INDEX_TYPE index = 0; index < EXCLUSIVE_END_INDEX; index++) {
        if (NS(SELF, get)(self, index)) {
            debug_fmt_print(fmt, stream, "(byte: %lu, offset: %lu, index: %lu)",
                            (size_t)DC_BITSET_STATIC_INDEX_TO_BYTES(index), (size_t)DC_BITSET_STATIC_INDEX_TO_OFFSET(index),
                            (size_t)index);
        }
    }
    fmt = debug_fmt_scope_end(fmt);
    debug_fmt_print(fmt, stream, "],\n");

    fmt = debug_fmt_scope_end(fmt);
    debug_fmt_print(fmt, stream, "}");
}

static SELF NS(SELF, clone)(SELF const* self) {
    INVARIANT_CHECK(self);
    SELF new_self = (SELF){
        .derive_c_bitset_static = gdb_marker_new(),
        .iterator_invalidation_tracker = mutation_tracker_new(),
    };
    memcpy(&new_self.bits, &self->bits, sizeof(self->bits));
    return new_self;
}

static INDEX_TYPE NS(SELF, size)(SELF const* self) {
    INDEX_TYPE size = 0;

    for (INDEX_TYPE byte = 0; byte < (INDEX_TYPE)DC_BITSET_STATIC_CAPACITY_TO_BYTES(EXCLUSIVE_END_INDEX); byte++) {
        size += __builtin_popcount((unsigned int)self->bits[byte]);
    }

    return size;
}

static void NS(SELF, delete)(SELF* self) { INVARIANT_CHECK(self); }

#define ITER_CONST NS(SELF, iter_const)
typedef struct {
    SELF const* bitset;
    INDEX_TYPE next_index;
    mutation_version version;
} ITER_CONST;
typedef INDEX_TYPE NS(ITER_CONST, item);

static bool NS(ITER_CONST, empty_item)(INDEX_TYPE const* item) {
    return *item == EXCLUSIVE_END_INDEX;
}

static INDEX_TYPE NS(ITER_CONST, next)(ITER_CONST* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);

    if (iter->next_index == EXCLUSIVE_END_INDEX) {
        return EXCLUSIVE_END_INDEX;
    }

    INDEX_TYPE next_index = iter->next_index;
    iter->next_index++;
    while (iter->next_index < EXCLUSIVE_END_INDEX &&
           !NS(SELF, get)(iter->bitset, iter->next_index)) {
        iter->next_index++;
    }
    return next_index;
}

static ITER_CONST NS(SELF, get_iter_const)(SELF const* self) {
    INVARIANT_CHECK(self);

    INDEX_TYPE next_index = 0;
    while (next_index < EXCLUSIVE_END_INDEX && !NS(SELF, get)(self, next_index)) {
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
    INDEX_TYPE next_index;
    mutation_version version;
} ITER;
typedef INDEX_TYPE NS(ITER, item);

static bool NS(ITER, empty_item)(INDEX_TYPE const* item) { return *item == EXCLUSIVE_END_INDEX; }

static INDEX_TYPE NS(ITER, next)(ITER* iter) {
    ASSUME(iter);
    mutation_version_check(&iter->version);

    if (iter->next_index == EXCLUSIVE_END_INDEX) {
        return EXCLUSIVE_END_INDEX;
    }

    INDEX_TYPE next_index = iter->next_index;
    iter->next_index++;
    while (iter->next_index < EXCLUSIVE_END_INDEX &&
           !NS(SELF, get)(iter->bitset, iter->next_index)) {
        iter->next_index++;
    }
    return next_index;
}

static ITER NS(SELF, get_iter)(SELF* self) {
    INVARIANT_CHECK(self);

    INDEX_TYPE next_index = 0;
    while (next_index < EXCLUSIVE_END_INDEX && !NS(SELF, get)(self, next_index)) {
        next_index++;
    }

    return (ITER){
        .bitset = self,
        .next_index = next_index,
        .version = mutation_tracker_get(&self->iterator_invalidation_tracker),
    };
}

#undef ITER

#undef INVARIANT_CHECK

#include <derive-c/core/index/bits_to_type/undef.h>
#include <derive-c/core/index/capacity_to_bits/undef.h>

#undef INDICES_CAPACITY
#undef EXCLUSIVE_END_INDEX

DC_TRAIT_BITSET(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/includes/undef.h>