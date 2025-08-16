#pragma once
#include <stddef.h>
#include <stdint.h>

#include <derive-c/core.h>
#include <derive-c/panic.h>

static inline bool is_power_of_2(size_t x) { return x != 0 && (x & (x - 1)) == 0; }

static inline size_t apply_capacity_policy(size_t capacity) {
    // TODO(oliverkillane): play with overallocation policy
    return next_power_of_2(capacity + (capacity / 2));
}

static inline size_t modulus_capacity(size_t index, size_t capacity) {
    DEBUG_ASSERT(is_power_of_2(capacity))
    // NOTE: If we know capacity is a power of 2, we can reduce the cost of 'index + 1 % capacity'
    return index & (capacity - 1);
}

static size_t const PROBE_DISTANCE = 1;
static size_t const INITIAL_CAPACITY = 32;