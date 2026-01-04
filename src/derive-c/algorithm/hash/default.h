#pragma once

#include <stdint.h>
#include <stddef.h>

#include <derive-c/core/compiler.h>

#include "id.h"
#include "fnv1a.h"

/// Sensible default hashers for standard data types
///  - Can be passed as a `<PARAM>_HASH` argument
#if defined DC_GENERIC_KEYWORD_SUPPORTED
    #define DC_DEFAULT_HASH(obj)                                                                   \
        _Generic(*(obj),                                                                           \
            int8_t: int8_t_hash_id,                                                                \
            uint8_t: uint8_t_hash_id,                                                              \
            int16_t: int16_t_hash_id,                                                              \
            uint16_t: uint16_t_hash_id,                                                            \
            int32_t: int32_t_hash_id,                                                              \
            uint32_t: uint32_t_hash_id,                                                            \
            int64_t: int64_t_hash_id,                                                              \
            uint64_t: uint64_t_hash_id,                                                            \
            const char*: dc_fnv1a_str)(obj)
#else
    #define DC_DEFAULT_HASH(obj)                                                                   \
        ([&]() -> std::uint64_t {                                                                  \
            using T = std::remove_cv_t<std::remove_reference_t<decltype(*(obj))>>;                 \
            if constexpr (std::is_same_v<T, std::int8_t>)                                          \
                return int8_t_hash_id(*(obj));                                                     \
            else if constexpr (std::is_same_v<T, std::uint8_t>)                                    \
                return uint8_t_hash_id(*(obj));                                                    \
            else if constexpr (std::is_same_v<T, std::int16_t>)                                    \
                return int16_t_hash_id(*(obj));                                                    \
            else if constexpr (std::is_same_v<T, std::uint16_t>)                                   \
                return uint16_t_hash_id(*(obj));                                                   \
            else if constexpr (std::is_same_v<T, std::int32_t>)                                    \
                return int32_t_hash_id(*(obj));                                                    \
            else if constexpr (std::is_same_v<T, std::uint32_t>)                                   \
                return uint32_t_hash_id(*(obj));                                                   \
            else if constexpr (std::is_same_v<T, std::int64_t>)                                    \
                return int64_t_hash_id(*(obj));                                                    \
            else if constexpr (std::is_same_v<T, std::uint64_t>)                                   \
                return uint64_t_hash_id(*(obj));                                                   \
            else if constexpr (std::is_same_v<T, char const*>)                                     \
                return dc_fnv1a_str(*(obj));                                                       \
            else {                                                                                 \
                static_assert(!std::is_same_v<T, T>, "DC_DEFAULT_HASH: unsupported type");         \
            }                                                                                      \
        }())
#endif