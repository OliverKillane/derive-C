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
typedef uint8_t _dc_ankerl_dfd;
static const _dc_ankerl_dfd _dc_ankerl_dfd_none = 0;
static const _dc_ankerl_dfd _dc_ankerl_dfd_max = (_dc_ankerl_dfd)UINT8_MAX;

DC_INTERNAL static _dc_ankerl_dfd _dc_ankerl_dfd_increment(_dc_ankerl_dfd dfd) {
    return (dfd == _dc_ankerl_dfd_max) ? _dc_ankerl_dfd_max : (_dc_ankerl_dfd)(dfd + 1U);
}

DC_INTERNAL static _dc_ankerl_dfd _dc_ankerl_dfd_decrement_for_backshift(_dc_ankerl_dfd dfd) {
    DC_ASSERT(dfd != _dc_ankerl_dfd_none);
    if (dfd == _dc_ankerl_dfd_max) {
        return _dc_ankerl_dfd_max;
    }
    return (_dc_ankerl_dfd)(dfd - 1U);
}

DC_INTERNAL static _dc_ankerl_dfd _dc_ankerl_dfd_new(uint8_t distance) {
    return (_dc_ankerl_dfd)(distance + 1U);
}

typedef struct {
    uint8_t fingerprint;
    _dc_ankerl_dfd dfd;
} _dc_ankerl_mdata;

DC_INTERNAL static bool _dc_ankerl_mdata_present(_dc_ankerl_mdata const* bucket) {
    return bucket->dfd != _dc_ankerl_dfd_none;
}

// Using small 32 bit buckets
typedef struct {
    _dc_ankerl_mdata mdata;
    uint16_t index;
} _dc_ankerl_small_bucket;

DC_STATIC_ASSERT(sizeof(_dc_ankerl_small_bucket) == 4);

DC_STATIC_CONSTANT size_t NS(_dc_ankerl_small_bucket, max_index_exclusive) = (size_t)UINT16_MAX;

DC_INTERNAL static _dc_ankerl_small_bucket NS(_dc_ankerl_small_bucket, new)(_dc_ankerl_mdata mdata,
                                                                            size_t index) {
    DC_ASSUME(index <= NS(_dc_ankerl_small_bucket, max_index_exclusive));
    return (_dc_ankerl_small_bucket){
        .mdata = mdata,
        .index = (uint16_t)(index),
    };
}

DC_INTERNAL static size_t NS(_dc_ankerl_small_bucket,
                             get_index)(_dc_ankerl_small_bucket const* bucket) {
    return (size_t)bucket->index;
}

// Using large 64 bit buckets
typedef struct {
    _dc_ankerl_mdata mdata;
    uint16_t index_hi;
    uint32_t index_lo;
} _dc_ankerl_bucket;

DC_STATIC_ASSERT(sizeof(_dc_ankerl_bucket) == 8);

DC_STATIC_CONSTANT size_t NS(_dc_ankerl_bucket, max_index_exclusive) = (size_t)UINT32_MAX +
                                                                       ((size_t)UINT16_MAX << 32);

DC_INTERNAL static _dc_ankerl_bucket NS(_dc_ankerl_bucket, new)(_dc_ankerl_mdata mdata,
                                                                size_t index) {
    DC_ASSUME(index <= NS(_dc_ankerl_bucket, max_index_exclusive));
    return (_dc_ankerl_bucket){
        .mdata = mdata,
        .index_hi = (uint16_t)(index >> 32),
        .index_lo = (uint32_t)index,
    };
}

DC_INTERNAL static size_t NS(_dc_ankerl_bucket, get_index)(_dc_ankerl_bucket const* bucket) {
    return (size_t)bucket->index_lo + ((size_t)bucket->index_hi << 32);
}
