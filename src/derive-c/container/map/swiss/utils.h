#pragma once

#include <derive-c/core/math.h>
#include <derive-c/core/prelude.h>
#include <stdint.h>

#define DC_SWISS_INITIAL_CAPACITY 256
#define _DC_SWISS_SIMD_PROBE_SIZE 16

// JUSTIFY: Why power of 2?
// - Has can be done with bitmasking, faster than modulus.
DC_STATIC_ASSERT(DC_MATH_IS_POWER_OF_2(_DC_SWISS_SIMD_PROBE_SIZE));

// clang-format off
#define DC_SWISS_VAL_ID_MASK  0b01111111
#define DC_SWISS_VAL_SENTINEL 0b11111111
#define DC_SWISS_VAL_DELETED  0b11111110
#define DC_SWISS_VAL_EMPTY    0b10000000
// clang-format on

typedef uint8_t dc_swiss_id;
typedef uint8_t dc_swiss_ctrl;

DC_INTERNAL DC_PURE static bool _dc_swiss_is_present(dc_swiss_ctrl ctrl) {
    switch (ctrl) {
    case DC_SWISS_VAL_EMPTY:
    case DC_SWISS_VAL_DELETED:
    case DC_SWISS_VAL_SENTINEL:
        return false;
    default:
        return true;
    }
}

DC_INTERNAL static uint8_t _dc_swiss_ctrl_get_id(dc_swiss_ctrl ctrl) {
    DC_ASSUME(_dc_swiss_is_present(ctrl));
    return ctrl & DC_SWISS_VAL_ID_MASK;
}

DC_INTERNAL static dc_swiss_id _dc_swiss_id_from_hash(size_t hash) {
    return (uint8_t)(hash & DC_SWISS_VAL_ID_MASK);
}

DC_INTERNAL static size_t dc_swiss_capacity(size_t for_items) {
    if (for_items < _DC_SWISS_SIMD_PROBE_SIZE) {
        return _DC_SWISS_SIMD_PROBE_SIZE;
    }
    return dc_math_next_power_of_2(for_items);
}

DC_INTERNAL static void _dc_swiss_ctrl_set_at(dc_swiss_ctrl* self, size_t capacity, size_t index,
                                              dc_swiss_ctrl val) {
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
    if (count >= max_load) {
        return DC_SWISS_DOUBLE_CAPACITY;
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
