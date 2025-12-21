#pragma once

#include <stdbool.h>
#include <stddef.h>

static inline bool dc_deque_rebalance_policy(size_t total_size, size_t front_size) {
    return total_size > 4 &&
           (front_size > total_size / 2 + 1 || (total_size - front_size) > total_size / 2 + 1);
}
