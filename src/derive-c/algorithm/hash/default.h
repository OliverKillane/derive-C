#pragma once

#include <stdint.h>
#include <stddef.h>

#include "id.h"
#include "fnv1a.h"

/// Sensible default hashers for standard data types
///  - Can be passed as a `<PARAM>_HASH` argument
#define DC_DEFAULT_HASH(obj)                                                                       \
    _Generic(*(obj),                                                                               \
        int8_t: int8_t_hash_id,                                                                    \
        uint8_t: uint8_t_hash_id,                                                                  \
        int16_t: int16_t_hash_id,                                                                  \
        uint16_t: uint16_t_hash_id,                                                                \
        int32_t: int32_t_hash_id,                                                                  \
        uint32_t: uint32_t_hash_id,                                                                \
        int64_t: int64_t_hash_id,                                                                  \
        uint64_t: uint64_t_hash_id,                                                                \
        const char*: dc_fnv1a_str)(obj)
