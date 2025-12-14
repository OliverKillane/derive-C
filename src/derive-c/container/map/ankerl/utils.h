#pragma once
#include <stdint.h>
#include <stddef.h>

#include <derive-c/core/math.h>
#include <derive-c/core/prelude.h>

static const size_t dc_ankerl_initial_items = 256;

static size_t dc_ankerl_buckets_capacity(size_t for_items) {
    if (for_items < dc_ankerl_initial_items) {
        return dc_ankerl_initial_items;
    }
    return dc_math_next_power_of_2(for_items);
}

static uint8_t dc_ankerl_fingerprint_from_hash(size_t hash) {
    uint8_t fp = (uint8_t)(hash >> (sizeof(size_t) * 8U - 8U));
    return (fp == 0) ? 1U : fp;
}

// The distance from desired (from initial hash) +1 so zeroed is empty
typedef uint8_t dc_ankerl_dfd;
static const dc_ankerl_dfd dc_ankerl_dfd_none = 0;
static const dc_ankerl_dfd dc_ankerl_dfd_max = (dc_ankerl_dfd)UINT8_MAX;

static dc_ankerl_dfd dc_ankerl_dfd_increment(dc_ankerl_dfd dfd) {
    return (dfd == dc_ankerl_dfd_max) ? dc_ankerl_dfd_max : (dc_ankerl_dfd)(dfd + 1U);
}

static dc_ankerl_dfd dc_ankerl_dfd_decrement_for_backshift(dc_ankerl_dfd dfd) {
    DC_ASSERT(dfd != dc_ankerl_dfd_none);
    if (dfd == dc_ankerl_dfd_max) {
        return dc_ankerl_dfd_max;
    }
    return (dc_ankerl_dfd)(dfd - 1U);
}

static dc_ankerl_dfd dc_ankerl_dfd_new(uint8_t distance) { return (dc_ankerl_dfd)(distance + 1U); }

typedef struct {
    uint8_t fingerprint;
    dc_ankerl_dfd dfd;
} dc_ankerl_mdata;

static bool dc_ankerl_mdata_present(dc_ankerl_mdata const* bucket) {
    return bucket->dfd != dc_ankerl_dfd_none;
}

typedef struct {
    uint16_t dense_index_hi;
    uint32_t dense_index_lo;
} __attribute__((packed)) dc_ankerl_index_large;

uint64_t const NS(dc_ankerl_index_large, max) = ((((int64_t)1) << 47) - 1);

DC_STATIC_ASSERT(sizeof(dc_ankerl_index_large) == 6);

static size_t NS(dc_ankerl_index_large, get)(dc_ankerl_index_large const* index) {
    return (size_t)index->dense_index_lo + ((size_t)index->dense_index_hi << 32);
}

static dc_ankerl_index_large NS(dc_ankerl_index_large, new)(size_t to) {
    DC_ASSERT(to <= dc_ankerl_index_large_max, "Index too large for ankerl");
    return (dc_ankerl_index_large){
        .dense_index_hi = (uint16_t)(to >> 32),
        .dense_index_lo = (uint32_t)to,
    };
}

typedef struct {
    uint16_t dense_index;
} dc_ankerl_index_small;

static uint64_t const NS(dc_ankerl_index_small, max) = UINT16_MAX;

DC_STATIC_ASSERT(sizeof(dc_ankerl_index_small) == 2);

static size_t NS(dc_ankerl_index_small, get)(dc_ankerl_index_small const* index) {
    return (size_t)index->dense_index;
}

static dc_ankerl_index_small NS(dc_ankerl_index_small, new)(size_t to) {
    DC_ASSERT(to <= dc_ankerl_index_small_max, "Index too large for ankerl map with small buckets");
    return (dc_ankerl_index_small){
        .dense_index = (uint16_t)to,
    };
}
