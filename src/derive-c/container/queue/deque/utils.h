#pragma once

#include <stdbool.h>
#include <stddef.h>

static inline bool _dc_deque_rebalance_policy(size_t total_size, size_t front_size,
                                              size_t back_size) {
    (void)total_size;
    // JUSTIFY: Simple lazy rebalance strategy
    //  - Simple & correct strategy.
    // TODO(oliverkillane): Consider how we could parameterize this for different applications
    return (front_size == 0 && back_size > 0) || (back_size == 0 && front_size > 0);
}
