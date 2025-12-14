#pragma once
#include <stdint.h>
#include <stddef.h>
#include <derive-c/core/math.h>

static const size_t dc_ankerl_initial_items = 256;

static uint8_t dc_ankerl_fingerprint_from_hash(size_t hash) {
    uint8_t fp = (uint8_t)(hash >> (sizeof(size_t) * 8U - 8U));
    return (fp == 0) ? 1U : fp;
}

static size_t dc_ankerl_buckets_capacity(size_t for_items) {
    if (for_items < dc_ankerl_initial_items) {
        return dc_ankerl_initial_items;
    }
    return dc_math_next_power_of_2(for_items);
}

// The distance from desired (from initial hash) +1 so zeroed is empty
typedef uint8_t dc_ankerl_dfd;
static const dc_ankerl_dfd dc_ankerl_dfd_none = 0;

static const dc_ankerl_dfd dc_ankerl_dfd_max = (dc_ankerl_dfd)UINT8_MAX;

static dc_ankerl_dfd dc_ankerl_dfd_increment(dc_ankerl_dfd dfd) {
    return (dfd == dc_ankerl_dfd_max) ? dc_ankerl_dfd_max : (dc_ankerl_dfd)(dfd + 1U);
}

static dc_ankerl_dfd dc_ankerl_dfd_decrement_for_backshift(dc_ankerl_dfd dfd) {
    ASSERT(dfd != dc_ankerl_dfd_none);

    // Capped/saturating dfd: max stays max so we don't underflow or lose the "capped" state.
    if (dfd == dc_ankerl_dfd_max) {
        return dc_ankerl_dfd_max;
    }

    // Normal case: shift one step closer to desired bucket.
    return (dc_ankerl_dfd)(dfd - 1U);
}

static dc_ankerl_dfd dc_ankerl_dfd_new(uint8_t distance) {
    return (dc_ankerl_dfd)(distance + 1U);
}

typedef struct {
    uint16_t dense_index_hi;
    uint32_t dense_index_lo;
}  __attribute__((packed)) dc_ankerl_index_large;

STATIC_ASSERT(sizeof(large_index) == 6);

typedef struct {
    uint16_t dense_index;
} small_index;

STATIC_ASSERT(sizeof(large_index) == 2);

typedef struct {
    uint8_t fingerprint;
    dc_ankerl_dfd dfd;
    uint16_t dense_index;
} x;

typedef struct {
    uint8_t fingerprint;
    dc_ankerl_dfd dfd;
    uint16_t dense_index;
} dc_ankerl_bucket_small;

STATIC_ASSERT(sizeof(dc_ankerl_bucket_small) == 4);

typedef struct {
    uint8_t fingerprint;
    dc_ankerl_dfd dfd;
    uint16_t dense_index_hi;
    uint32_t dense_index_lo;
} dc_ankerl_bucket_big;

STATIC_ASSERT(sizeof(dc_ankerl_bucket_big) == 8);

static bool dc_ankerl_bucket_big_present(dc_ankerl_bucket_big const* bucket) {
    return bucket->dfd != dc_ankerl_dfd_none;
}

static size_t dc_ankerl_bucket_get_index(dc_ankerl_bucket_big const* bucket) {
    return (size_t)bucket->dense_index_lo + ((size_t)bucket->dense_index_hi << 32);
}

static void dc_ankerl_bucket_big_set_index(dc_ankerl_bucket_big* bucket, size_t index) {
    bucket->dense_index_lo = (uint32_t)index;
    bucket->dense_index_hi = (uint16_t)(index >> 32);
}

static void dc_ankerl_bucket_big_set(dc_ankerl_bucket_big* bucket,
                                     uint8_t fingerprint,
                                     dc_ankerl_dfd dfd,
                                     size_t index) {
    bucket->fingerprint = fingerprint;
    bucket->dfd = dfd;
    dc_ankerl_bucket_big_set_index(bucket, index);
}

