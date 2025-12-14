#pragma once
#include <stddef.h>
#include <stdint.h>

#include <derive-c/core/prelude.h>

static inline size_t dc_apply_capacity_policy(size_t capacity) {
    // TODO(oliverkillane): play with overallocation policy
    return dc_math_next_power_of_2(capacity + (capacity / 2));
}

static size_t const DC_INITIAL_CAPACITY = 32;
