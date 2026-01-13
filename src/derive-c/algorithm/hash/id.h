#pragma once

#include <derive-c/core/prelude.h>
#include <derive-c/core/std/reflect.h>

/// No hashing, just returns the integer value.
/// For most circumstances with a key as a single integer, this is a good option.
#define ID(type, ...)                                                                              \
    static size_t type##_hash_id(type const* key) {                                                \
        DC_STATIC_ASSERT(sizeof(type) <= sizeof(size_t),                                           \
                         "ID hashing only supports up to size_t integers");                        \
        return (size_t)(*key);                                                                     \
    }

DC_INT_REFLECT(ID)

#undef ID
