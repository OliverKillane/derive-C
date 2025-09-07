#pragma once
#include <stddef.h>

#include <derive-c/core/panic.h>

static inline bool circular_realloc_policy(size_t total_capacity, size_t size) {
    return total_capacity == size;
}
