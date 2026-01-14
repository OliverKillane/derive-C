#pragma once

#include <derive-c/core/prelude.h>
#include <derive-c/core/std/reflect.h>

/// The worst possible hash, for testing purposes.
#define WORST(type, ...)                                                                           \
    DC_PUBLIC static size_t type##_hash_worst(type const* key) {                                   \
        (void)key;                                                                                 \
        return 0;                                                                                  \
    }

DC_INT_REFLECT(WORST)

#undef WORST
