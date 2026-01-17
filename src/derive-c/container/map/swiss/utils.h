#pragma once

#if !defined(__SSE2__)
    #error "swiss table requires SSE2"
#endif

#include <stdint.h>
#include <emmintrin.h>

#include <derive-c/core/math.h>
#include <derive-c/core/prelude.h>

#define DC_SWISS_INITIAL_CAPACITY 256

// JUSTIFY: Using 16 byte type, rather than 32 byte
//  - Arbitrarily chose to stick with 16 byte.
// TODO(oliverkillane): Experiment with 32 byte groups
typedef __m128i _dc_swiss_ctrl_group;
typedef uint8_t _dc_swiss_ctrl;

// Indexes of groups, and offset within a group
typedef size_t _dc_swiss_ctrl_group_index;
typedef uint16_t _dc_swiss_ctrl_group_offset;

// When searching a group, we get a bitmask back
typedef uint16_t _dc_swiss_ctrl_group_bitmask;

#define _DC_SWISS_SIMD_PROBE_SIZE sizeof(_dc_swiss_ctrl_group)

// JUSTIFY: Why power of 2?
// - Has can be done with bitmasking, faster than modulus.
DC_STATIC_ASSERT(DC_MATH_IS_POWER_OF_2(_DC_SWISS_SIMD_PROBE_SIZE));

// clang-format off
#define DC_SWISS_VAL_SENTINEL 0b11111111
#define DC_SWISS_VAL_DELETED  0b11111110
#define DC_SWISS_VAL_EMPTY    0b10000000
// clang-format on

DC_INTERNAL DC_PURE static bool _dc_swiss_is_present(_dc_swiss_ctrl ctrl) {
    switch (ctrl) {
    case DC_SWISS_VAL_EMPTY:
    case DC_SWISS_VAL_DELETED:
    case DC_SWISS_VAL_SENTINEL:
        return false;
    default:
        return true;
    }
}

DC_INTERNAL static _dc_swiss_ctrl _dc_swiss_ctrl_from_hash(size_t hash) {
    // Taking the top 7 bits for `H2`
    return (uint8_t)(hash >> (sizeof(size_t) * 8 - 7));
}

DC_INTERNAL static size_t dc_swiss_capacity(size_t for_items) {
    if (for_items < _DC_SWISS_SIMD_PROBE_SIZE) {
        return _DC_SWISS_SIMD_PROBE_SIZE;
    }
    return dc_math_next_power_of_2(for_items);
}

DC_INTERNAL static void _dc_swiss_ctrl_set_at(_dc_swiss_ctrl* self, size_t capacity, size_t index,
                                              _dc_swiss_ctrl val) {
    // JUSTIFY: Setting index past capacity
    // ctrl[..capacity) = (empty or value)
    // ctrl[capacity] = SENTINEL
    // ctrl[capacity+1+i] = ctrl[i] for (_DC_SWISS_PROBE_SIZE - 1)
    self[index] = val;
    if (index < (_DC_SWISS_SIMD_PROBE_SIZE - 1)) {
        self[capacity + 1 + index] = val;
    }
}

typedef enum {
    DC_SWISS_DOUBLE_CAPACITY,
    DC_SWISS_CLEANUP_TOMBSONES,
    DC_SWISS_DO_NOTHING,
} _dc_swiss_rehash_action;

DC_INTERNAL static _dc_swiss_rehash_action
_dc_swiss_heuristic_should_extend(size_t tombstones, size_t count, size_t capacity) {
    DC_ASSUME(capacity > 0);

    const size_t max_load = capacity - (capacity / 8);

    if (count + tombstones >= max_load) {
        return tombstones > 0 ? DC_SWISS_CLEANUP_TOMBSONES : DC_SWISS_DOUBLE_CAPACITY;
    }

    if (tombstones > (count / 2)) {
        return DC_SWISS_CLEANUP_TOMBSONES;
    }

    return DC_SWISS_DO_NOTHING;
}

#define _DC_SWISS_NO_INDEX ((size_t)-1)
typedef size_t _dc_swiss_optional_index;

// JUSTIFY: Not just size_t
//  - We need to store the NONE index (as the max index)
//  - We have a buffer at the end of the table
static size_t const _dc_swiss_index_capacity = SIZE_MAX - (1 + _DC_SWISS_SIMD_PROBE_SIZE);

DC_INTERNAL static _dc_swiss_ctrl_group _dc_swiss_group_load(const _dc_swiss_ctrl* group_ptr) {
    // Deal with unaligned loads - can be optimised away in release
    return _mm_loadu_si128((const __m128i_u*)group_ptr);
}

DC_INTERNAL static _dc_swiss_ctrl_group_bitmask _dc_swiss_group_match(_dc_swiss_ctrl_group group,
                                                                      uint8_t value) {
    __m128i cmp = _mm_cmpeq_epi8(group, _mm_set1_epi8((char)value));
    return (_dc_swiss_ctrl_group_bitmask)_mm_movemask_epi8(cmp);
}

DC_INTERNAL static _dc_swiss_ctrl_group_offset
_dc_swiss_ctrl_group_bitmask_lowest(_dc_swiss_ctrl_group_bitmask mask) {
    DC_ASSUME(mask != 0);
    return (_dc_swiss_ctrl_group_offset)__builtin_ctz(mask);
}

DC_INTERNAL static _dc_swiss_ctrl_group_bitmask
_dc_swiss_ctrl_group_bitmask_clear_lowest(_dc_swiss_ctrl_group_bitmask mask) {
    return mask & (mask - 1);
}

// After getting the matches in a group, iterate on the matches
#define _DC_SWISS_BITMASK_FOR_EACH(mask, idx_var)                                                  \
    for (_dc_swiss_ctrl_group_bitmask _m = (mask); _m != 0;                                        \
         _m = _dc_swiss_ctrl_group_bitmask_clear_lowest(_m))                                       \
        for (_dc_swiss_ctrl_group_offset idx_var = _dc_swiss_ctrl_group_bitmask_lowest(_m),        \
                                         _once = 1;                                                \
             _once; _once = 0)

DC_INTERNAL static size_t _dc_swiss_group_index_to_slot(size_t group_start,
                                                        _dc_swiss_ctrl_group_offset idx,
                                                        size_t capacity) {
    size_t pos = group_start + idx;
    if (pos < capacity) {
        return pos;
    }
    if (pos == capacity) {
        // On the sentinel value
        return _DC_SWISS_NO_INDEX;
    }

    // After the sentinel
    return pos - capacity - 1;
}
