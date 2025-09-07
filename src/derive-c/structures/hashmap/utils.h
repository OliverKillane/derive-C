#pragma once
#include <stddef.h>
#include <stdint.h>

#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>

static inline size_t apply_capacity_policy(size_t capacity) {
    // TODO(oliverkillane): play with overallocation policy
    return next_power_of_2(capacity + (capacity / 2));
}

static size_t const PROBE_DISTANCE = 1;
static size_t const INITIAL_CAPACITY = 32;
