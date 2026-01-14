#pragma once

#include <stdint.h>
#include <stddef.h>

#include <derive-c/core/math.h>
#include <derive-c/core/prelude.h>

static const size_t dc_ankerl_initial_items = 256;

DC_INTERNAL static size_t _dc_ankerl_buckets_capacity(size_t for_items) {
    if (for_items < dc_ankerl_initial_items) {
        return dc_ankerl_initial_items;
    }
    return dc_math_next_power_of_2(for_items);
}

DC_INTERNAL static uint8_t _dc_ankerl_fingerprint_from_hash(size_t hash) {
    uint8_t fp = (uint8_t)(hash >> (sizeof(size_t) * 8U - 8U));
    return (fp == 0) ? 1U : fp;
}

// The distance from desired (from initial hash) +1 so zeroed is empty
typedef uint8_t dc_ankerl_dfd;
static const dc_ankerl_dfd _dc_ankerl_dfd_none = 0;
static const dc_ankerl_dfd _dc_ankerl_dfd_max = (dc_ankerl_dfd)UINT8_MAX;

DC_INTERNAL static dc_ankerl_dfd _dc_ankerl_dfd_increment(dc_ankerl_dfd dfd) {
    return (dfd == _dc_ankerl_dfd_max) ? _dc_ankerl_dfd_max : (dc_ankerl_dfd)(dfd + 1U);
}

DC_INTERNAL static dc_ankerl_dfd _dc_ankerl_dfd_decrement_for_backshift(dc_ankerl_dfd dfd) {
    DC_ASSERT(dfd != _dc_ankerl_dfd_none);
    if (dfd == _dc_ankerl_dfd_max) {
        return _dc_ankerl_dfd_max;
    }
    return (dc_ankerl_dfd)(dfd - 1U);
}

DC_INTERNAL static dc_ankerl_dfd _dc_ankerl_dfd_new(uint8_t distance) {
    return (dc_ankerl_dfd)(distance + 1U);
}

typedef struct {
    uint8_t fingerprint;
    dc_ankerl_dfd dfd;
} dc_ankerl_mdata;

DC_INTERNAL static bool _dc_ankerl_mdata_present(dc_ankerl_mdata const* bucket) {
    return bucket->dfd != _dc_ankerl_dfd_none;
}
